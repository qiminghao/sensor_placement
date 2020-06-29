cmake_minimum_required(VERSION 3.15)
project(sensor_placement C)

set(CMAKE_C_STANDARD 99)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,1073741824")

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/lib)
link_libraries(epanet2)

add_executable(sensor_placement src/main.c)
target_link_libraries(sensor_placement epanet2)