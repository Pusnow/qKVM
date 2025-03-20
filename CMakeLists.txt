cmake_minimum_required(VERSION 3.16)

project(qKVM VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia SerialPort)
qt_standard_project_setup()

qt_add_executable(qKVM
    main.cpp
)

target_link_libraries(qKVM PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::SerialPort)

set_target_properties(qKVM PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)