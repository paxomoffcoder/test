cmake_minimum_required(VERSION 3.16)
project(DashboardClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Charts)

add_executable(dashboard_client
    main.cpp
    authwindow.h
    authwindow.cpp
    mainwindow.h
    mainwindow.cpp
    networkclient.h
    networkclient.cpp
    jsontablemodel.h
    jsontablemodel.cpp
)

target_link_libraries(dashboard_client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Charts
)
