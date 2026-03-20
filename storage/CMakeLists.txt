cmake_minimum_required(VERSION 3.16)

project(CSVCompressor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

qt_add_executable(CSVCompressor
    ${PROJECT_SOURCES}
)

target_link_libraries(CSVCompressor PRIVATE Qt6::Widgets Qt6::Core)

set_target_properties(CSVCompressor PROPERTIES
    WIN32_EXECUTABLE TRUE
)