
cmake_minimum_required(VERSION 3.16)

project(AbrikosovSimulation
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib QUIET)

if(NOT raylib_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)
    set(RAYLIB_TARGET PkgConfig::RAYLIB)
else()
    set(RAYLIB_TARGET raylib)
endif()

add_executable(AbrikosovSimulation
    src/main.cpp
    src/Vortex.cpp
    src/Simulation.cpp
    src/Renderer.cpp
)

target_include_directories(AbrikosovSimulation
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(AbrikosovSimulation
    PRIVATE
    ${RAYLIB_TARGET}
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(AbrikosovSimulation PRIVATE
        -Wall
        -Wextra
        -O2
    )
endif()
