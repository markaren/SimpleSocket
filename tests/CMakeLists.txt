
add_executable(test_tcp test_tcp.cpp)
target_link_libraries(test_tcp PRIVATE simple_socket Catch2::Catch2WithMain)


add_executable(test_udp test_udp.cpp)
target_link_libraries(test_udp PRIVATE simple_socket Catch2::Catch2WithMain)

add_subdirectory(integration)
