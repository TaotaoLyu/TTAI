cmake_minimum_required(VERSION 3.16)
project(http_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mysql_pool
    src/db_connection.cpp
    src/db_connection.cpp
    src/db_connection_pool.cpp
)

target_include_directories(mysql_pool
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

target_link_libraries(mysql_pool
    PUBLIC 
        muduo_base
        mysqlcppconn
)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "Building http_server as standalone project")
    add_executable(mysql_pool_test
        test/test_main.cpp
    )
    target_link_libraries(mysql_pool_test
        PRIVATE
            mysql_pool
    )
endif()

# [ cloc --include-lang="HTML,JavaScript,CSS" . ]  [ cloc --include-lang="C,C++,C/C++ Header" . ]