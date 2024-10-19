set(publicHeaderDir "${PROJECT_SOURCE_DIR}/include")

set(publicHeaders

        "simple_socket/SimpleConnection.hpp"
        "simple_socket/TCPSocket.hpp"
        "simple_socket/UDPSocket.hpp"
        "simple_socket/UnixDomainSocket.hpp"
        "simple_socket/WebSocket.hpp"

        "simple_socket/modbus/HoldingRegister.hpp"
        "simple_socket/modbus/ModbusClient.hpp"
        "simple_socket/modbus/modbus_helper.hpp"
        "simple_socket/modbus/ModbusServer.hpp"

        "simple_socket/util/byte_conversion.hpp"
        "simple_socket/util/port_query.hpp"
)

set(privateHeaders
        "simple_socket/socket_common.hpp"
        "simple_socket/Socket.hpp"
        "simple_socket/util/uuid.hpp"

        "simple_socket/ws/WebSocketConnection.hpp"
        "simple_socket/ws/WebSocketHandshake.hpp"
        "simple_socket/ws/WebSocketHandshakeKeyGen.hpp"
)

set(sources
        "simple_socket/util/port_query.cpp"
        "simple_socket/TCPSocket.cpp"
        "simple_socket/UDPSocket.cpp"
        "simple_socket/UnixDomainSocket.cpp"

        "simple_socket/ws/WebSocket.cpp"
        "simple_socket/ws/WebSocketClient.cpp"

        "simple_socket/modbus/HoldingRegister.cpp"
        "simple_socket/modbus/ModbusClient.cpp"
        "simple_socket/modbus/ModbusServer.cpp"
)

if (WIN32)
    list(APPEND publicHeaders
            "simple_socket/Pipe.hpp"
    )

    list(APPEND privateHeaders
            "simple_socket/WSASession.hpp"
    )

    list(APPEND sources
            "simple_socket/Pipe.cpp"
    )
endif ()


set(publicHeadersFull)
foreach (header IN LISTS publicHeaders)
    list(APPEND publicHeadersFull "${publicHeaderDir}/${header}")
endforeach ()


add_library(simple_socket "${publicHeadersFull}" "${privateHeaders}" "${sources}")
target_compile_features(simple_socket PUBLIC "cxx_std_20")
if (UNIX)
    set_target_properties(simple_socket PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif ()
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
