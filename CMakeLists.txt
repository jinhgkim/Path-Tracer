cmake_minimum_required(VERSION 3.16)
project(PathTracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
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
    if (NOT APPLE)
        message(FATAL_ERROR "The GPU path tracer requires Apple's Metal framework.")
    endif()

    enable_language(OBJCXX)

    add_executable(gpu_pt src/gpu/main.cpp)
    set_source_files_properties(src/gpu/main.cpp PROPERTIES LANGUAGE OBJCXX)
    target_include_directories(gpu_pt PRIVATE metal-cpp)

    # default.metallib must sit beside the executable. The generator expression
    # stops multi-config generators appending a per-config subdirectory.
    set_target_properties(gpu_pt PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_BINARY_DIR}>")

    find_library(PT_METAL_FRAMEWORK Metal)
    find_library(PT_FOUNDATION_FRAMEWORK Foundation)
    find_library(PT_QUARTZCORE_FRAMEWORK QuartzCore)
    target_link_libraries(gpu_pt PRIVATE
        ${PT_METAL_FRAMEWORK} ${PT_FOUNDATION_FRAMEWORK} ${PT_QUARTZCORE_FRAMEWORK})

    # --- Metal shaders ---
    # Shaders.metal is the only translation unit
    set(PT_METAL_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/gpu/Shaders")
    set(PT_METAL_AIR "${CMAKE_CURRENT_BINARY_DIR}/Shaders.air")
    set(PT_METALLIB "${CMAKE_CURRENT_BINARY_DIR}/default.metallib")

    file(GLOB PT_METAL_SHADERS CONFIGURE_DEPENDS "${PT_METAL_SHADER_DIR}/*.metal")

    add_custom_command(
        OUTPUT "${PT_METALLIB}"
        COMMAND xcrun -sdk macosx metal -c "${PT_METAL_SHADER_DIR}/Shaders.metal"
                -o "${PT_METAL_AIR}"
        COMMAND xcrun -sdk macosx metallib "${PT_METAL_AIR}" -o "${PT_METALLIB}"
        MAIN_DEPENDENCY "${PT_METAL_SHADER_DIR}/Shaders.metal"
        DEPENDS ${PT_METAL_SHADERS}
        BYPRODUCTS "${PT_METAL_AIR}"
        COMMENT "Compiling Metal shaders into default.metallib"
        VERBATIM)

    add_custom_target(metal_shaders DEPENDS "${PT_METALLIB}")
    add_dependencies(gpu_pt metal_shaders)
endif()
