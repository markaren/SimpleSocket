# SimpleSocket

A simple cross-platform communication library for C++ (no external dependencies) 
for education and hobby usage.

Supports:
* UDP
* TCP/IP
* Unix Domain Sockets
* WebSockets
* HTTP fetcher
* Modbus [TCP]
* MQTT [TCP, WebSockets]


SimpleSocket supports TLS via openssl. 
This feature is optional and can be enabled at build time and allows use of TLS for TCP/IP connections,
including Secure WebSockets (wss://), MQTT and https:// support in the HTTP fetcher.

### Downstream usage with CMake FetchContent
```cmake
include(FetchContent)
set(SIMPLE_SOCKET_BUILD_TESTS OFF)
set(SIMPLE_SOCKET_WITH_TLS OFF/ON)
set(SIMPLE_SOCKET_WITH_MQTT ON/OFF)
set(SIMPLE_SOCKET_WITH_MODBUS ON/OFF)
set(SIMPLE_SOCKET_WITH_MEMORY ON/OFF)
set(SIMPLE_SOCKET_WITH_WEBSOCKETS ON/OFF)
FetchContent_Declare(
        SimpleSocket
        GIT_REPOSITORY https://github.com/markaren/SimpleSocket.git
        GIT_TAG tag_or_commit_hash
)
FetchContent_MakeAvailable(SimpleSocket)

target_link_libraries(some_target PRIVATE simple_socket)
```
