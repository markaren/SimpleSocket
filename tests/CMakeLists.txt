
add_executable(test_tcp test_tcp.cpp)
add_test(NAME test_tcp COMMAND test_tcp)
target_link_libraries(test_tcp PRIVATE simple_socket Catch2::Catch2WithMain)

add_executable(test_udp test_udp.cpp)
add_test(NAME test_udp COMMAND test_udp)
target_link_libraries(test_udp PRIVATE simple_socket Catch2::Catch2WithMain)

add_executable(test_un test_un.cpp)
add_test(NAME test_un COMMAND test_un)
target_link_libraries(test_un PRIVATE simple_socket Catch2::Catch2WithMain)

if (SIMPLE_SOCKET_WITH_WEBSOCKETS)
    add_executable(test_ws test_ws.cpp)
    add_test(NAME test_ws COMMAND test_ws)
    target_link_libraries(test_ws PRIVATE simple_socket Catch2::Catch2WithMain)

    if (SIMPLE_SOCKET_WITH_TLS)
        add_executable(test_wss_client test_wss_client.cpp)
        add_test(NAME test_wss_client COMMAND test_wss_client)
        target_link_libraries(test_wss_client PRIVATE simple_socket Catch2::Catch2WithMain)
    endif ()
endif ()

add_executable(test_conversion test_conversion.cpp)
add_test(NAME test_conversion COMMAND test_conversion)
target_link_libraries(test_conversion PRIVATE simple_socket Catch2::Catch2WithMain)

if (SIMPLE_SOCKET_WITH_MODBUS)
    add_executable(test_modbus test_modbus.cpp)
    add_test(NAME test_modbus COMMAND test_conversion)
    target_link_libraries(test_modbus PRIVATE simple_socket Catch2::Catch2WithMain)
endif ()

add_executable(test_port_query test_port_query.cpp)
add_test(NAME test_port_query COMMAND test_port_query)
target_link_libraries(test_port_query PRIVATE simple_socket Catch2::Catch2WithMain)


if (SIMPLE_SOCKET_WITH_MEMORY)
    add_executable(test_shared_memory test_shared_memory.cpp)
    target_link_libraries(test_shared_memory PRIVATE simple_socket Catch2::Catch2WithMain)
    add_test(NAME test_shared_memory COMMAND test_shared_memory)
endif ()


if (UNIX)
    target_link_libraries(test_tcp PRIVATE pthread)
    target_link_libraries(test_udp PRIVATE pthread)
    target_link_libraries(test_un PRIVATE pthread)
endif ()

add_subdirectory(integration)
