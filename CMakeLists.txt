cmake_minimum_required(VERSION 3.10)
project(MyGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

include_directories(include)

add_executable(MyGame ${SOURCES})

# Enable warnings
if (MSVC)
    target_compile_options(MyGame PRIVATE /W4)
else()
    target_compile_options(MyGame PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Platform-specific raylib linking
if (WIN32)
    target_include_directories(MyGame PRIVATE
        ${CMAKE_SOURCE_DIR}/include/raylib-5.5_win64_msvc16
    )
    target_link_directories(MyGame PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/raylib-5.5_win64_msvc16
    )
    target_link_libraries(MyGame PRIVATE raylib)
else()
    find_package(raylib REQUIRED)
    target_link_libraries(MyGame PRIVATE raylib)
endif()

# Copy resources directory after build
add_custom_command(TARGET MyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:MyGame>/resources)
