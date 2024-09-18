
add_executable(test_tcp test_tcp.cpp)
add_test(NAME test_tcp COMMAND test_tcp)
target_link_libraries(test_tcp PRIVATE simple_socket Catch2::Catch2WithMain)

add_executable(test_udp test_udp.cpp)
add_test(NAME test_udp COMMAND test_udp)
target_link_libraries(test_udp PRIVATE simple_socket Catch2::Catch2WithMain)

add_executable(test_un test_un.cpp)
add_test(NAME test_un COMMAND test_un)
target_link_libraries(test_un PRIVATE simple_socket Catch2::Catch2WithMain)

if (UNIX)
    target_link_libraries(test_tcp PRIVATE pthread)
    target_link_libraries(test_udp PRIVATE pthread)
    target_link_libraries(test_un PRIVATE pthread)

endif ()

add_subdirectory(integration)
