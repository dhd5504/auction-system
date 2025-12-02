cmake_minimum_required(VERSION 3.10)
project(tcp_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)

add_executable(tcp_server
    src/main.cpp
    src/database.cpp
    src/session_manager.cpp
    src/packet_router.cpp
    src/packet_io.cpp
    src/tcp_server.cpp
)

target_include_directories(tcp_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol
)

target_link_libraries(tcp_server PRIVATE
    SQLite::SQLite3
    pthread
)
