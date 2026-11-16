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

# Копируем шрифт с поддержкой кириллицы рядом с исполняемым файлом.
add_custom_command(TARGET AbrikosovSimulation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AbrikosovSimulation>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/DejaVuSans.ttf"
        "$<TARGET_FILE_DIR:AbrikosovSimulation>/assets/DejaVuSans.ttf"
)

if(MSVC)
    target_compile_options(AbrikosovSimulation PRIVATE /utf-8)
endif()
