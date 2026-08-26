# GPU Path Tracer

A GPU-accelerated path tracer written in C++ using **Metal** via [metal-cpp](https://developer.apple.com/metal/cpp/).


## Prerequisites

The GPU path tracer requires macOS with the Xcode command line tools, which
provide `xcrun`, the Metal compiler, and the Metal framework:

```bash
xcode-select --install
```

`metal-cpp` is vendored in this repo, so there is nothing else to install.

## Build

From the directory containing `Shaders.metal`:

```bash
xcrun -sdk macosx metal -c Shaders.metal -o Shaders.air
xcrun -sdk macosx metallib Shaders.air -o default.metallib
```

to compile the Metal shader (`Shaders.metal`) into a `.metallib`.

From the project root directory:

```bash
cmake -S . -B build -DBUILD_GPU_PT=ON
cmake --build build
```

to build the c++ code.

## Run

Move `default.metallib` to the `build` directory so the executable can find it, and run the following from the project root directory:

```bash
./build/gpu_pt > output.ppm
```

## References

- [GPU Programming with the Metal Shading Language](https://www.youtube.com/watch?v=VQK28rRK6OU): A very good intro video on GPU programming with Metal
- [Accelerated Ray Tracing in One Weekend in CUDA](https://developer.nvidia.com/blog/accelerated-ray-tracing-cuda)

# CPU Path Tracer

A CPU-based path tracer written in C++, based on [_Ray Tracing in One Weekend_](https://raytracing.github.io/books/RayTracingInOneWeekend.html), and extended with multithreading support.

![final scene](https://github.com/jinhgkim/Path-Tracer/blob/main/img/final_scene.png)

## Final Render Statistics

- **Resolution:** `1200 × 675`
- **Samples per Pixel:** `500`
- **Machine:** Apple MacBook Pro (M1 chip, 8 cores)
- **Render Time:**
  - **Single-threaded:** 6h 6m 44s
  - **Multi-threaded (8 cores):** 53m 42s
  - **Multi-threaded (8 cores) + `BVH`:** 2m 12s
  

## Prerequisites

The CPU path tracer uses [oneTBB](https://github.com/uxlfoundation/oneTBB) for
multithreading:

```bash
brew install tbb
```

## Build

From the project root directory:

```bash
cmake -S . -B build -DBUILD_CPU_PT=ON
cmake --build build
```

to build the c++ code.

## Run

```bash
./build/cpu_pt > output.ppm
```
