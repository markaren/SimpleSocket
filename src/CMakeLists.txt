
add_library(simple_socket socket.cpp UDPSocket.cpp)
target_compile_features(simple_socket PUBLIC "cxx_std_17")

if (WIN32)
    target_link_libraries(simple_socket ws2_32)
endif ()


target_include_directories(simple_socket
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
