
add_executable(run_server run_server.cpp)
target_link_libraries(run_server PRIVATE simple_socket)

add_executable(run_client run_client.cpp)
target_link_libraries(run_client PRIVATE simple_socket)
