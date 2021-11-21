cmake_minimum_required(VERSION 3.20)
project(crop_to_valid_extent)

set(CMAKE_CXX_STANDARD 17)

add_executable(crop_to_valid_extent main.cpp)
target_link_libraries(crop_to_valid_extent gdal)