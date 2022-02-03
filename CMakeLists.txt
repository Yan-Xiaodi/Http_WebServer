cmake_minimum_required(VERSION 3.0.2)
project(WebServer)

set(CMAKE_CXX_STANDARD 11)
set(debug 1)
if(debug)
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    message(debug)
else()
    set(CMAKE_BUILD_TYPE Release)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    message(release)
endif()

include_directories(./include /usr/local/mysql/include)

add_executable(server src/main.cpp src/timerHeap.cpp src/http_conn.cpp
                                src/log.cpp src/sql_connection_pool.cpp 
                                src/webserver.cpp src/config.cpp)
target_link_libraries(server -lpthread -lmysqlclient)