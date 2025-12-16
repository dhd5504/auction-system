cmake_minimum_required(VERSION 3.5)

project(server_app VERSION 0.1 LANGUAGES CXX)

set(APP_TARGET server_app)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql)

set(SOURCES
    main.cpp
    network/TcpServer.h
    network/TcpServer.cpp
    network/ClientSession.h
    network/ClientSession.cpp
    db/Database.h
    db/Database.cpp
    protocol/Protocol.h
    protocol/CommandHandler.cpp
)

add_executable(${APP_TARGET} ${SOURCES})

target_include_directories(${APP_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/db
    ${CMAKE_CURRENT_SOURCE_DIR}/protocol
)

target_link_libraries(${APP_TARGET} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
)

install(TARGETS ${APP_TARGET}
    RUNTIME DESTINATION bin
)
