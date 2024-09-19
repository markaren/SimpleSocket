set(publicHeaderDir "${PROJECT_SOURCE_DIR}/include")

set(publicHeaders
        "simple_socket/port_query.hpp"
        "simple_socket/TCPSocket.hpp"
        "simple_socket/UDPSocket.hpp"
        "simple_socket/WebSocket.hpp"
)

set(publicHeadersFull)
foreach (header IN LISTS publicHeaders)
    list(APPEND publicHeadersFull "${publicHeaderDir}/${header}")
endforeach ()

set(privateHeaders
        "simple_socket/common.hpp"
        "simple_socket/WebSocketHandshake.hpp"
        "simple_socket/WSASession.hpp"
)

set(sources
        "simple_socket/port_query.cpp"
        "simple_socket/TCPSocket.cpp"
        "simple_socket/UDPSocket.cpp"
        "simple_socket/WebSocket.cpp"
)

add_library(simple_socket "${publicHeadersFull}" "${privateHeaders}" "${sources}")
target_compile_features(simple_socket PUBLIC "cxx_std_17")

if (WIN32)
    target_link_libraries(simple_socket "ws2_32")
endif ()


target_include_directories(simple_socket
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)
