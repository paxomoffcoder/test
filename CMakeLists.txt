cmake_minimum_required(VERSION 3.16)
project(test_llm_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql Concurrent)

add_library(llm_foundation STATIC
    llm_config.cpp
    ollama_provider.cpp
    context_builder.cpp
    llm_service.cpp
)

target_include_directories(llm_foundation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(llm_foundation PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Concurrent
)

add_executable(llm_smoke_test
    smoke_test.cpp
)

target_link_libraries(llm_smoke_test
    llm_foundation
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

add_executable(runtime_config_smoke_test
    runtime_config_smoke_test.cpp
)

target_link_libraries(runtime_config_smoke_test
    llm_foundation
    Qt${QT_VERSION_MAJOR}::Core
)
