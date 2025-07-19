# Path Tracer

![final scene](https://github.com/jinhgkim/Path-Tracer/blob/main/img/final_scene.png)

## Final Render Statistics

- **Resolution:** `1200 × 675`
- **Samples per Pixel:** `500`
- **Machine:** Apple MacBook Pro (M1 chip, 8 cores)
- **Render Time:**
  - **Single-threaded:** 6h 6m 44s
  - **Multithreaded (8 cores):** 53m 42s

This image was rendered using a CPU-based path tracer written in C++, following the approach in [_Ray Tracing in One Weekend_](https://raytracing.github.io/books/RayTracingInOneWeekend.html).

The renderer now supports **multithreading** via C++17 parallel algorithms (`std::execution::par`) and Intel TBB, achieving over **6× speedup** by parallelizing scanline rendering.

# GPU Path Tracer (WIP)

This project includes a GPU-accelerated path tracer written using **Metal** via `metal-cpp`.

### Compiling Metal Shaders

To compile the Metal shader (`Shaders.metal`) into a `.metallib` for use in the application, run:

```bash
xcrun -sdk macosx metal -c Shaders.metal -o Shaders.air
xcrun -sdk macosx metallib Shaders.air -o default.metallib
```

### 📚 Resources I Found Useful for GPU Programming

- [GPU Programming with the Metal Shading Language](https://www.youtube.com/watch?v=VQK28rRK6OU): A very good intro video on GPU programming with Metal
- [Accelerated Ray Tracing in One Weekend in CUDA](https://developer.nvidia.com/blog/accelerated-ray-tracing-cuda)
