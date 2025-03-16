#include <iostream>
#include <string>
#include <sstream>
#include <regex>
#include <chrono>
using time_point = std::chrono::system_clock::time_point;
static time_point begin_time;

static bool is_exit = false;
static int ret_code = 0;
static int socket_fd;
static int client_fd;


// platform specific configs
#if defined(_LINUX) || defined(__linux__)
    
    #include <unistd.h>
    #include <signal.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    
    #define S_PLATFORM_INIT
    
    #define S_socklen socklen_t
    
    #define S_socket socket
    #define S_close close
    #define S_shutdown shutdown
    #define S_SHUT_RDWR SHUT_RDWR
    #define S_setsockopt setsockopt
    #define S_bind bind
    #define S_getsockname getsockname
    #define S_accept accept
    #define S_send send
    
    // sleep in miliseconds implemented using nanosleep()
    #include <time.h>
    #include <errno.h>
    int milisleep(long msec)
    {
        struct timespec ts;
        int res;
        
        if (msec < 0)
        {
            errno = EINVAL;
            return -1;
        }
        
        ts.tv_sec = msec / 1000;
        ts.tv_nsec = (msec % 1000) * 1000000;
        
        do {
            res = nanosleep(&ts, &ts);
        } while (res && errno == EINTR);
        
        return res;
    }
    #define sleep milisleep
    
    void exit_handler(int signal)
    {
        is_exit = true;
        switch (signal)
        {
        case SIGINT:
        case SIGTERM:
        case SIGKILL:
            S_close(client_fd);
            client_fd = -1;
            S_close(socket_fd);
            socket_fd = -1;
            return;
        default:
            return;
        }
    }
    
    bool create_exit_handler()
    {
        struct sigaction sa;
        sa.sa_handler = exit_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGKILL, &sa, NULL);
        return true;
    }
    
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__CYGWIN__)
    
    #include <winsock2.h>
    #include <windows.h>
    
    #define S_PLATFORM_INIT \
    WSADATA wsa_data; \
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) \
    { \
        fprintf(stderr, "Failed to initialize Winsock.\n"); \
        return -1; \
    }
    
    #define S_socklen int
    
    #define S_socket socket
    #define S_close closesocket
    #define S_shutdown shutdown
    #define S_SHUT_RDWR SD_BOTH
    #define S_setsockopt setsockopt
    #define S_bind bind
    #define S_getsockname getsockname
    #define S_accept accept
    #define S_send send
    
    #define sleep Sleep
    
    BOOL WINAPI exit_handler(DWORD signal)
    {
        is_exit = true;
        switch (signal)
        {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            S_close(client_fd);
            client_fd = -1;
            S_close(socket_fd);
            socket_fd = -1;
            return TRUE;
        
        default:
            return FALSE;
        }
    }
    
    bool create_exit_handler()
    {
        return static_cast<bool>(SetConsoleCtrlHandler(exit_handler, TRUE));
    }
    
#endif


// Error codes
#undef INVALID_SOCKET
#define INVALID_SOCKET          -1

#define SOCKET_INIT_FAILED      -2
#define INVALID_ARGUMENT        -3



// response protocol
#if defined(RESP_TCP) && defined(RESP_HTTP)
#undef RESP_TCP
#undef RESP_HTTP
#define RESP_TCP
#elif !defined(RESP_TCP) && !defined(RESP_HTTP)
#define RESP_TCP
#endif



std::string create_response(const std::string& data_in)
{

#if defined(RESP_TCP)
    
    return data_in;
    
#elif defined(RESP_HTTP)
    
    std::ostringstream oss;
    oss
        << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: application/json; charset=UTF-8\r\n"
        << "Content-Length: " << data_in.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << data_in
        << "\r\n\r\n"
    ;
    return oss.str();
    
#endif
}

std::string collect_system_metrics()
{
    time_point current_time = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - begin_time);
    
    auto duration_hr = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= std::chrono::duration_cast<std::chrono::milliseconds>(duration_hr);
    
    auto duration_min = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= std::chrono::duration_cast<std::chrono::milliseconds>(duration_min);
    
    auto duration_sec = std::chrono::duration_cast<std::chrono::seconds>(duration);
    duration -= std::chrono::duration_cast<std::chrono::milliseconds>(duration_sec);
    
    
    std::ostringstream oss;
    oss << "{\"uptime\":\""
        << duration_hr.count() << ":"
        << duration_min.count() << ":"
        << duration_sec.count() << ":"
        << duration.count()
    << "\"}";
    
    return oss.str();
}

int run_server(
    const unsigned long address, const unsigned short port,
    const unsigned int allowed_connection
)
{
    begin_time = std::chrono::system_clock::now();
    
    S_PLATFORM_INIT
    
    if (!create_exit_handler())
    {
        std::cerr << "Failed to create server exit handler." << std::endl;
        S_close(socket_fd);
        return SOCKET_INIT_FAILED;
    }
    
    // create TCP socket
    socket_fd = S_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket." << std::endl;
        S_close(socket_fd);
        return SOCKET_INIT_FAILED;
    }
    
    // set socket option to reuse address
    // this is for force linux kernel to reuse a socket_fd while its closed and in TIME_WAIT state
    int opt = 1;
    if (S_setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&opt), sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options." << std::endl;
        S_close(socket_fd);
        return SOCKET_INIT_FAILED;
    }
    
    // setup address & port
    struct sockaddr_in socket_addr;
    socket_addr.sin_family = AF_INET; // IPv4
    socket_addr.sin_addr.s_addr = htonl(address);
    socket_addr.sin_port = htons(port);
    
    // bind socket address to fd
    if (S_bind(socket_fd, reinterpret_cast<struct sockaddr*>(&socket_addr), sizeof(socket_addr)) == INVALID_SOCKET)
    {
        std::cerr << "Failed to bind address." << std::endl;
        S_close(socket_fd);
        return SOCKET_INIT_FAILED;
    }
    
    // listening to address:port, and only allow specified amount of connection
    if (listen(socket_fd, allowed_connection) == INVALID_SOCKET)
    {
        std::cerr << "Failed to set listen." << std::endl;
        S_close(socket_fd);
        return SOCKET_INIT_FAILED;
    }
    
    // get real address:port of server
    struct sockaddr_in server_addr;
    S_socklen len = sizeof(server_addr);
    if (S_getsockname(socket_fd, reinterpret_cast<struct sockaddr*>(&server_addr), &len) == INVALID_SOCKET)
    {
        std::cerr << "Failed to get sockname." << std::endl;
        S_close(socket_fd);
        return SOCKET_INIT_FAILED;
    }
    int server_port = ntohs(server_addr.sin_port);
    char* server_ip = inet_ntoa(socket_addr.sin_addr);
    std::cout << "Socket init completed: " << server_ip << ":" << server_port << std::endl;
    
    
    // main loop
    while (!is_exit)
    {
        // accept new connection
        struct sockaddr_in client_sk;
        S_socklen client_sk_len = sizeof(client_sk);
        client_fd = S_accept(socket_fd, reinterpret_cast<struct sockaddr*>(&client_sk), &client_sk_len);
        
        // server will wait on S_accept(), so when we leave above line,
        // we first check whether server get terminated
        if (is_exit)
        {
            return ret_code;
        }
        
        if (client_fd == INVALID_SOCKET)
        {
            std::cerr << "Failed to accept connection." << std::endl;
            continue;
        }
        
        // get client address & port
        char *client_addr = inet_ntoa(client_sk.sin_addr);
        int client_port = ntohs(client_sk.sin_port);
        std::cout << "Client connected. IP address: " << client_addr << ":" << client_port << std::endl;
        
        // respond
        std::string buffer(create_response(collect_system_metrics()));
        
        int bytes_send = S_send(client_fd, buffer.data(), buffer.size(), 0);
        // error on sending data
        if (bytes_send <= 0)
            std::cerr << "Failed to send data." << std::endl;
        std::cout << "Send " << buffer.size() << " bytes." << std::endl;
        
        
#ifndef NO_CLOSING_DELAY
        // add a small delay before closing,
        // so client would have time to retrieve all data
        sleep(100);
#endif
        
        // shutdonw & close client connection
        S_shutdown(client_fd, S_SHUT_RDWR);
        S_close(client_fd);
    }
    
    // close socket
    S_close(socket_fd);
    
    return 0;
}

int addr_to_uint32(const std::string &addr, uint32_t &addr_out)
{
    // handle special case and return host order result
    if (addr == "loopback" || addr == "localhost")
    {
        addr_out = INADDR_LOOPBACK;
        return 0;
    }
    else if (addr == "any")
    {
        addr_out = INADDR_ANY;
        return 0;
    }
    
    // use regex to parse IPv4 address
    std::regex rgx(R"((\d+)\.(\d+)\.(\d+)\.(\d+))");
    std::smatch matches;
    uint32_t output = 0;
    
    // if regex match the address, convert it to host order address
    if (std::regex_search(addr, matches, rgx))
    {
        uint8_t octet;
        for (size_t i = 1; i < matches.size(); ++i)
        {
            octet = static_cast<uint8_t>(std::stoi(matches[i].str()));
            output |= octet << (8 * (4 - i));
        }
    }
    // otherwise return
    else
    {
        return -1;
    }
    
    // return host order result
    addr_out = output;
    return 0;
}




int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout << "Usage: heartbeat [ADDRESS] [PORT]" << std::endl;
        return 0;
    }
    
    try
    {
        std::string address(argv[1]);
        uint32_t address_i;
        if (0 != addr_to_uint32(address, address_i))
        {
            return INVALID_ARGUMENT;
        }
        uint32_t port = std::stoi(argv[2]);
        return run_server(address_i, port, 1);
    }
    catch(const std::invalid_argument& invarg)
    {
        std::cerr << "Invalid argument" << std::endl;
        if (client_fd >= 0) S_close(client_fd);
        if (socket_fd >= 0) S_close(socket_fd);
        return INVALID_ARGUMENT;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        if (client_fd >= 0) S_close(client_fd);
        if (socket_fd >= 0) S_close(socket_fd);
    }
    
}
