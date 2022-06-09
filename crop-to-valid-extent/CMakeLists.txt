cmake_minimum_required(VERSION 3.20)
project(crop_to_valid_extent)

set(CMAKE_CXX_STANDARD 17)

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_compile_options(-O3)
endif ()

add_executable(crop-to-valid-extent main.cpp ValidRegion-impl.cpp Region.cpp)
target_link_libraries(crop-to-valid-extent gdal)
