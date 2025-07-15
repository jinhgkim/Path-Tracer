cmake_minimum_required(VERSION 3.16)
project(PathTracer LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# === Options ===
option(BUILD_CPU_PT "Build the CPU path-tracer executable" OFF)
option(BUILD_GPU_PT "Build the GPU path-tracer executable" OFF)


# === CPU Path Tracer ===
if (BUILD_CPU_PT)
    find_package(TBB REQUIRED) 
    add_executable(cpu_pt src/cpu/main.cpp)
    target_link_libraries(cpu_pt PRIVATE TBB::tbb)
endif()
 

# === GPU Path Tracer === 
if (BUILD_GPU_PT)
    include_directories(metal-cpp)

    add_executable(gpu_pt src/gpu/main.cpp)

    set_source_files_properties(src/gpu/main.cpp PROPERTIES LANGUAGE OBJCXX)

    find_library(METAL Metal)
    find_library(FOUNDATION Foundation)
    find_library(QUARTZCORE QuartzCore)
 
    target_link_libraries(gpu_pt PRIVATE "-framework Metal" "-framework Foundation" "-framework QuartzCore")

endif()
