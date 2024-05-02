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
        GIT_TAG tag_or_commit_hash
)
FetchContent_MakeAvailable(SimpleSocket)

target_link_libraries(some_target PRIVATE simple_socket)
```
