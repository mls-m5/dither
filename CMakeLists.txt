
cmake_minimum_required(VERSION 3.23)
project(dither)

add_subdirectory(lib/sdlpp)

add_executable(
    dither
    src/main.cpp
    )

target_compile_features(dither PRIVATE cxx_std_17)

list(APPEND
    CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/sdlpp/cmake/sdl2)
find_package(SDL2_image REQUIRED)

target_link_libraries(
    dither
    PUBLIC
    sdlpp
    SDL2::Image
    )

file(
    COPY data
    DESTINATION .)
