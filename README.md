# heartbeat

A super simple C++ implementation of heartbeat server. Supporting both TCP and HTTP response.

# compile

### Linux
```sh
g++ -o heartbeat heartbeat.cpp
```

### Windows
```pwsh
# MinGW
g++ -o heartbeat.exe heartbeat.cpp -lws2_32

#MSVC
cl.exe /Fe:heartbeat.exe heartbeat.cpp ws2_32.lib /EHsc
```

### Makefile (Support all platforms)
```sh
make
```

### CMake (Support all platforms)

#### Generate build system
```sh
cmake -S . -B build
```

#### Build the project
```sh
cmake --build
```

> [!NOTE]
> In MSVC's case, you can use `/DRESP_TCP` syntax to add additional compiler flags.

> [!NOTE]
> In Makefile's case, you can use parameters after `make` command to add compiler flags.
> e.g.: `make RESP=TCP`, `make RESP=HTTP`, `make CLOSING_DELAY=0`

> [!NOTE]
> In CMake's case, you can use parameters after `cmake -S . -B build` command to add compiler flags.
> e.g.: `-DRESP=TCP`, `-DRESP=HTTP`, `-DCLOSING_DELAY=ON`

You can add an additional compiler flag to change response protocol
* `-DRESP_TCP`: default response protocol, raw TCP data
* `-DRESP_HTTP`: response data using HTTP
* `-DNO_CLOSING_DELAY`: server will add a small delay before closing, so client would have time to retrieve all data. This flag will disable such behavior.
    * recommend to leave it enabled to avoid wired error of client.

# sample response

```json
{"uptime":"0:0:11:435"}
```

Response data is a json where the value of `uptime` is in format:

```
dd:mm:ss:ms
```
