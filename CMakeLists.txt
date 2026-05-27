cmake_minimum_required(VERSION 3.16)
project(test_llm_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_library(llm_foundation STATIC
    llm_config.cpp
)

target_include_directories(llm_foundation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(llm_foundation PUBLIC Qt${QT_VERSION_MAJOR}::Core)
