cmake_minimum_required(VERSION 3.5)

project(api LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network HttpServer)

add_executable(api
  main.cpp
)
target_link_libraries(api
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
)

include(GNUInstallDirs)
install(TARGETS api
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
