cmake_minimum_required(VERSION 3.10)
project(gateway)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(PISTACHE_LIB NAMES pistache REQUIRED)

add_executable(gateway
    src/main.cpp
    src/tcp_client.cpp
    api/auth_api.cpp
    api/product_api.cpp
    api/room_api.cpp
    ws/ws_server.cpp
)

target_include_directories(gateway PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/ws
    ${CMAKE_CURRENT_SOURCE_DIR}/../protocol
)

target_link_libraries(gateway PRIVATE
    ${PISTACHE_LIB}
    pthread
)
