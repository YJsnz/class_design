cmake_minimum_required(VERSION 3.16)
project(SpreadsheetApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Charts Network)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(SpreadsheetApp
    src/main.cpp
    src/Spreadsheet.cpp
    src/Spreadsheet.h
    src/Cell.cpp
    src/Cell.h
    src/FormulaParser.cpp
    src/FormulaParser.h
    src/FileFormat.cpp
    src/FileFormat.h
)

target_link_libraries(SpreadsheetApp PRIVATE Qt6::Widgets Qt6::Core Qt6::Charts Qt6::Network)