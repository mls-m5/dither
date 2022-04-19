
cmake_minimum_required(VERSION 3.23)
project(dither)

add_executable(
    dither
    src/main.cpp
    )

target_compile_features(dither PRIVATE cxx_std_17)
