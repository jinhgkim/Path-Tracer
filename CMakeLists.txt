cmake_minimum_required(VERSION 3.16)
project(PathTracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()


# === Options ===
option(BUILD_CPU_PT "Build the CPU path-tracer executable" ON)
option(BUILD_GPU_PT "Build the GPU path-tracer executable" OFF)

option(ENABLE_OSL "Compile the OSL shading backend into the CPU path tracer" OFF)

if (ENABLE_OSL AND NOT BUILD_CPU_PT)
    message(FATAL_ERROR
        "ENABLE_OSL requires -DBUILD_CPU_PT=ON; OSL shading is built into cpu_pt.")
endif()


# === CPU Path Tracer ===
if (BUILD_CPU_PT)
    find_package(TBB CONFIG QUIET)
    if (NOT TBB_FOUND)
        message(FATAL_ERROR
            "TBB not found. The CPU path tracer needs oneTBB for multithreading.\n"
            "Install it with:  brew install tbb\n")
    endif()

    add_executable(cpu_pt src/cpu/main.cpp)
    target_include_directories(cpu_pt PRIVATE src/cpu)
    target_link_libraries(cpu_pt PRIVATE TBB::tbb)

    # --- Optional OSL shading backend ---
    if (ENABLE_OSL)
        find_package(OSL CONFIG REQUIRED)

        set(PT_OSL_SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/cpu/osl/shaders")
        set(PT_OSL_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
        file(MAKE_DIRECTORY "${PT_OSL_SHADER_DIR}")

        # CONFIGURE_DEPENDS re-globs at build time, so a new shader needs no re-run.
        file(GLOB OSL_SHADER_SOURCES CONFIGURE_DEPENDS "${PT_OSL_SHADER_SRC_DIR}/*.osl")
        file(GLOB OSL_SHADER_HEADERS CONFIGURE_DEPENDS "${PT_OSL_SHADER_SRC_DIR}/*.h")

        set(OSL_COMPILED_SHADERS)
        foreach(shader IN LISTS OSL_SHADER_SOURCES)
            get_filename_component(shader_name "${shader}" NAME_WE)
            set(compiled "${PT_OSL_SHADER_DIR}/${shader_name}.oso")

            add_custom_command(
                OUTPUT "${compiled}"
                COMMAND "$<TARGET_FILE:OSL::oslc>"
                    -I "${PT_OSL_SHADER_SRC_DIR}" -o "${compiled}" "${shader}"
                MAIN_DEPENDENCY "${shader}"
                DEPENDS ${OSL_SHADER_HEADERS} OSL::oslc
                COMMENT "Compiling OSL shader ${shader_name}.osl"
                VERBATIM)

            list(APPEND OSL_COMPILED_SHADERS "${compiled}")
        endforeach()

        add_custom_target(osl_shaders DEPENDS ${OSL_COMPILED_SHADERS})
        add_dependencies(cpu_pt osl_shaders)

        target_sources(cpu_pt PRIVATE
            src/cpu/osl/renderer.cpp
            src/cpu/osl/shading.cpp)

        target_compile_definitions(cpu_pt PRIVATE
            PT_USE_OSL
            PT_OSL_SHADER_PATH="${PT_OSL_SHADER_DIR}")

        # OSL::oslexec supplies its OpenImageIO and Imath dependencies.
        target_link_libraries(cpu_pt PRIVATE OSL::oslexec)

        # liboslexec may use OSL's vendored pugixml outside the install tree.
        # OSLConfig.cmake defines OSL_LIB_DIR.
        get_filename_component(PT_OSL_SOURCE_DIR "${OSL_LIB_DIR}/../.." ABSOLUTE)
        # Search only OSL paths so a system pugixml is not added to the rpath.
        find_library(PT_OSL_PUGIXML_LIBRARY pugixml
            PATHS "${OSL_LIB_DIR}" "${PT_OSL_SOURCE_DIR}/ext/dist/lib"
            NO_DEFAULT_PATH)
        if (PT_OSL_PUGIXML_LIBRARY)
            get_filename_component(PT_PUGIXML_DIR "${PT_OSL_PUGIXML_LIBRARY}" DIRECTORY)
        endif()

        set(PT_OSL_EXTRA_RPATHS "" CACHE STRING
            "Extra rpath entries for cpu_pt, for dylibs found nowhere else")

        set(PT_OSL_RPATHS ${PT_PUGIXML_DIR} ${PT_OSL_EXTRA_RPATHS})
        if (PT_OSL_RPATHS)
            list(REMOVE_DUPLICATES PT_OSL_RPATHS)
            set_property(TARGET cpu_pt PROPERTY BUILD_RPATH "${PT_OSL_RPATHS}")
        endif()
    endif()
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
