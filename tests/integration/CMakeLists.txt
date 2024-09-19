
add_executable(run_tcp_server run_tcp_server.cpp)
target_link_libraries(run_tcp_server PRIVATE simple_socket)

add_executable(run_tcp_client run_tcp_client.cpp)
target_link_libraries(run_tcp_client PRIVATE simple_socket)

add_executable(run_ws run_ws.cpp)
target_link_libraries(run_ws PRIVATE simple_socket)

add_executable(run_pipe_server run_pipe_server.cpp)
target_link_libraries(run_pipe_server PRIVATE simple_socket)

add_executable(run_pipe_client run_pipe_client.cpp)
target_link_libraries(run_pipe_client PRIVATE simple_socket)

if (UNIX)
    target_link_libraries(run_tcp_server PRIVATE pthread)
    target_link_libraries(run_tcp_client PRIVATE pthread)
    target_link_libraries(run_ws PRIVATE pthread)
endif ()
