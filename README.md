# SimpleSocket

A simple cross-platform Socket implementation for C++ (no external dependencies) 
for education and hobby usage.


### Downstream usage with CMake FetchContent
```cmake
include(FetchContent)
set(SIMPLE_SOCKET_BUILD_TESTS OFF)
FetchContent_Declare(
        SimpleSocket
        GIT_REPOSITORY https://github.com/markaren/SimpleSocket.git
        GIT_TAG a80b83260ec8bb598d0dd018d035685375eb3e5f
)
FetchContent_MakeAvailable(SimpleSocket)

target_link_libraries(<target> PRIVATE simple_socket)
```
