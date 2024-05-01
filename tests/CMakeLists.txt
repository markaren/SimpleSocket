
add_executable(test_socketlib test_socketlib.cpp)
target_link_libraries(test_socketlib PRIVATE simple_socket Catch2::Catch2WithMain)

add_subdirectory(integration)
