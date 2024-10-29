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

You can add an additional compiler flag to change response protocol
* `-DRESP_TCP`: default response protocol, raw TCP data
* `-DRESP_HTTP`: response data using HTTP
* `-DNO_CLOSING_DELAY`: server will add a small delay before closing, so client would have time to retrieve all data. This flag will disable such behavior.
    * recommend to leave it enabled to avoid wired error of client.

> [!NOTE]
> In MSVC's case, you can use `/DRESP_TCP` syntax to add additional compiler flags.

> [!NOTE]
> In Makefile's case, you can use parameters after `make` command to add compiler flags.
> e.g.: `make RESP=TCP`, `make RESP=HTTP`, `make CLOSING_DELAY=0`


# sample response
```json
{"uptime":21.0301}
```
