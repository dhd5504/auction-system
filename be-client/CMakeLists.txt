cmake_minimum_required(VERSION 3.10)
project(be_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)
find_library(PISTACHE_LIB NAMES pistache REQUIRED)

add_executable(be_client
    main.cpp
    api/product_api.cpp
    api/room_api.cpp
    db/sqlite.cpp
    tcp/tcp_client.cpp
    ws/ws_server.cpp
)

target_include_directories(be_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/db
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp
    ${CMAKE_CURRENT_SOURCE_DIR}/ws
    ${CMAKE_CURRENT_SOURCE_DIR}/model
)

target_link_libraries(be_client PRIVATE
    ${PISTACHE_LIB}
    SQLite::SQLite3
    pthread
)
