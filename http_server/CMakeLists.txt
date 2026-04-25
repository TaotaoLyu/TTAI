cmake_minimum_required(VERSION 3.16)
project(http_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(http_server
    src/http_server/http_server.cpp
    src/http_server/http_context.cpp 
    src/ssl/ssl_context.cpp
    src/ssl/ssl_connection.cpp
    src/ssl/ssl_config.cpp
    src/router/router.cpp
)

target_include_directories(http_server
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/http_server
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ssl
        ${CMAKE_CURRENT_SOURCE_DIR}/include/router
)

target_link_libraries(http_server
    PUBLIC 
        muduo_net
        muduo_base
        pthread
        ssl
        crypto
)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "Building http_server as standalone project")
    add_executable(http_server_test
        test/test_main.cpp
    )
    target_link_libraries(http_server_test
        PRIVATE
            http_server
    )
endif()

# [ cloc --include-lang="HTML,JavaScript,CSS" . ]  [ cloc --include-lang="C,C++,C/C++ Header" . ]