
add_executable(run_tcp_server run_tcp_server.cpp)
target_link_libraries(run_tcp_server PRIVATE simple_socket)

add_executable(run_tcp_client run_tcp_client.cpp)
target_link_libraries(run_tcp_client PRIVATE simple_socket)

add_executable(run_ws run_ws.cpp)
target_link_libraries(run_ws PRIVATE simple_socket)

add_executable(run_wss run_wss.cpp)
target_link_libraries(run_wss PRIVATE simple_socket)

add_executable(modbus_client modbus_client.cpp)
target_link_libraries(modbus_client PRIVATE simple_socket)

add_executable(modbus_server modbus_server.cpp)
target_link_libraries(modbus_server PRIVATE simple_socket)

add_executable(memory_server memory_server.cpp)
target_link_libraries(memory_server PRIVATE simple_socket)

add_executable(memory_client memory_client.cpp)
target_link_libraries(memory_client PRIVATE simple_socket)

if (UNIX)
    target_link_libraries(run_tcp_server PRIVATE pthread)
    target_link_libraries(run_tcp_client PRIVATE pthread)
    target_link_libraries(run_ws PRIVATE pthread)
endif ()

file(COPY ws_client.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
