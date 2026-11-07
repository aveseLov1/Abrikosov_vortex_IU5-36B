cmake_minimum_required(VERSION 3.16)
project(AbrikosovSimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG REQUIRED)

add_executable(AbrikosovSimulation
    src/main.cpp
    src/Renderer.cpp
    src/Simulation.cpp
    src/Vortex.cpp
    src/Magnet.cpp
)

target_include_directories(AbrikosovSimulation PRIVATE include)
target_link_libraries(AbrikosovSimulation PRIVATE raylib)

if(UNIX AND NOT APPLE)
    target_link_libraries(AbrikosovSimulation PRIVATE m)
endif()
