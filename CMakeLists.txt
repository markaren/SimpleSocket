cmake_minimum_required(VERSION 3.19)
project(simple_socket VERSION 0.3.0)

option(SIMPLE_SOCKET_BUILD_TESTS OFF)
option(SIMPLE_SOCKET_WITH_TLS "Enable TLS (OpenSSL) for WSS" OFF)

set(CMAKE_CXX_STANDARD 20)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

if(SIMPLE_SOCKET_WITH_TLS)
    find_package(OpenSSL REQUIRED)
endif ()

add_subdirectory(src)

if (SIMPLE_SOCKET_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)

    enable_testing()
    add_subdirectory(tests)
endif ()

configure_package_config_file(cmake/config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/simple_socket-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/simple_socket"
        PATH_VARS SIMPLE_SOCKET_WITH_TLS
        NO_SET_AND_CHECK_MACRO)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/simple_socket-config-version.cmake"
        VERSION ${simple_socket_VERSION}
        COMPATIBILITY SameMajorVersion)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/simple_socket-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/simple_socket-config-version.cmake"
        DESTINATION
        "${CMAKE_INSTALL_DATADIR}/simple_socket")

install(TARGETS simple_socket EXPORT simple_socket-targets)
install(DIRECTORY "include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(EXPORT simple_socket-targets
        NAMESPACE simple_socket::
        DESTINATION "${CMAKE_INSTALL_DATADIR}/simple_socket")
