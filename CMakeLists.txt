cmake_minimum_required(VERSION 3.10)
project(MyGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default build type is debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

add_executable(MyGame ${SOURCES})

target_include_directories(MyGame PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/scenes
)


if (MSVC)
    target_compile_options(MyGame PRIVATE /W4)

    target_include_directories(MyGame SYSTEM PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/raylib-5.5_win64_msvc16/include
    )
    target_link_directories(MyGame PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/raylib-5.5_win64_msvc16/lib
    )
    target_link_libraries(MyGame PRIVATE raylib)
else()
    target_compile_options(MyGame PRIVATE 
        -Wall -Wextra -Wpedantic
        -Wno-error
        -Wno-reorder
        -Wno-unused-parameter
        -Wno-missing-field-initializers
    )

    target_include_directories(MyGame SYSTEM PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/raylib-5.5_win64_mingw-w64/include
    )
    target_link_directories(MyGame PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/raylib-5.5_win64_mingw-w64/lib
    )
    target_link_libraries(MyGame PRIVATE raylib winmm)
endif()

# Copy resources after build
add_custom_command(TARGET MyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:MyGame>/resources)
