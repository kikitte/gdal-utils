find_package(GDAL REQUIRED)

add_executable(generate_random_points
        generate_random_points.cpp
        ../source/roi.cpp)

target_link_libraries(generate_random_points ${GDAL_LIBRARIES})