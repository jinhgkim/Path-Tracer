# GPU Path Tracer (WIP)

A GPU-accelerated path tracer written in C++ using **Metal** via [`metal-cpp`](https://developer.apple.com/metal/cpp/).

![gpu scene](https://github.com/jinhgkim/Path-Tracer/blob/main/img/gpu_material.png)

## Build

To compile the Metal shader (`Shaders.metal`) into a `.metallib`:

```bash
// From the directory containing Shaders.metal
xcrun -sdk macosx metal -c Shaders.metal -o Shaders.air
xcrun -sdk macosx metallib Shaders.air -o default.metallib
```

To build the c++ code:

```bash
// From the build dir
cmake .. -DBUILD_GPU_PT=ON
make
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

Parallelized with C++17 parallel algorithms (`std::execution::par`) and Intel TBB, achieving over **6× speedup** in scanline rendering.
