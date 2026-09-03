cmake_minimum_required(VERSION 3.16)
project(PathTracer LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release )
endif()


# === Options ===
option(BUILD_CPU_PT "Build the CPU path-tracer executable" OFF)
option(BUILD_GPU_PT "Build the GPU path-tracer executable" OFF)


# === CPU Path Tracer ===
if (BUILD_CPU_PT)
    find_package(TBB QUIET)
    if (NOT TBB_FOUND)
        message(FATAL_ERROR
            "TBB not found. The CPU path tracer needs oneTBB for multithreading.\n"
            "Install it with:  brew install tbb\n")
    endif()
    add_executable(cpu_pt src/cpu/main.cpp)
    target_link_libraries(cpu_pt PRIVATE TBB::tbb)
endif()
 

# === GPU Path Tracer === 
if (BUILD_GPU_PT)
    add_executable(gpu_pt src/gpu/main.cpp)
    target_include_directories(gpu_pt PRIVATE metal-cpp)

    set_source_files_properties(src/gpu/main.cpp PROPERTIES LANGUAGE OBJCXX)

    find_library(METAL Metal)
    find_library(FOUNDATION Foundation)
    find_library(QUARTZCORE QuartzCore)
 
    target_link_libraries(gpu_pt PRIVATE ${METAL} ${FOUNDATION} ${QUARTZCORE})

endif()
