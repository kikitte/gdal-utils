cmake_minimum_required(VERSION 3.1)

project(random_points)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# global compiler option
if (${CMAKE_BUILD_TYPE} STREQUAL Release)
    add_compile_options(-O3)
endif ()

include_directories(deps)

add_subdirectory(apps)

