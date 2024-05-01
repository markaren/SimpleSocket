
set(sources
    "Socket.cpp"
)

add_library(simple_socket Socket.cpp)
target_include_directories(simple_socket
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if (WIN32)
    target_link_libraries(simple_socket ws2_32)
endif ()
