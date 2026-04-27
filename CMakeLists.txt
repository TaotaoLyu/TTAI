cmake_minimum_required(VERSION 3.16)
project(TTAI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(http_server)
add_subdirectory(mysql_pool)

add_library(ttai
    TTAI/src/chat_server.cpp 
    TTAI/src/handlers/ai_home_page.cpp 
    TTAI/src/handlers/ai_login.cpp 
    TTAI/src/handlers/ai_register.cpp 
    TTAI/src/user/user_service.cpp 
)

target_include_directories(ttai
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/TTAI/include/user
        ${CMAKE_CURRENT_SOURCE_DIR}/TTAI/include/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/TTAI/include
)

target_link_libraries(ttai
    PUBLIC
        http_server
        mysql_pool
)

add_executable(TTAI
    TTAI/app/main.cpp
)

target_link_libraries(TTAI
    PRIVATE
        ttai
)