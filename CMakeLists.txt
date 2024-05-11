cmake_minimum_required(VERSION 3.28)
project(ovalToRaster)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

find_package(doctest REQUIRED)
include_directories(/usr/local/include)

add_executable(ovalToRaster main.cpp
        ovalRasterizer.cpp
        ovalRasterizer.h)

add_executable(ovalToRasterTest test_ovalRasterizer.cpp
        ovalRasterizer.cpp
        ovalRasterizer.h)

target_compile_definitions(ovalToRasterTest PRIVATE TESTING)

target_link_libraries(ovalToRaster
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
)

