cmake_minimum_required(VERSION 3.10)
project(PathTracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(TBB REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb)