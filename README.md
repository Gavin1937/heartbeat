# heartbeat

A super simple C++ implementation of heartbeat server. Supporting both TCP and HTTP response.

# compile

### Linux
```sh
g++ -o heartbeat heartbeat.cpp
```

### Windows
```sh
g++ -o heartbeat heartbeat.cpp -lws2_32
```

You can add an additional compiler flag to change response protocol
* `-DRESP_TCP`: default response protocol, raw TCP data
* `-DRESP_HTTP`: response data using HTTP
* `-DNO_CLOSING_DELAY`: server will add a small delay before closing, so client would have time to retrieve all data. This flag will disable such behavior.
    * recommend to leave it enabled to avoid wired error of client.

# sample response
```json
{"uptime":21.0301}
```
