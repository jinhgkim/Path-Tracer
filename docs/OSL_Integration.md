# OSL Integration

This document explains how the CPU path tracer connects Open Shading Language
(OSL) shaders to its existing material interface and C++ integrator. See the
[README](../README.md#cpu-path-tracer) for build and run instructions.

The integration supports the three materials used in the sphere scene from
*Ray Tracing in One Weekend*, which is also the source of the `rtiow` prefix
used throughout this document. It adapts OSL to this renderer rather than
implementing a general-purpose OSL backend.

## Integration Overview

The path tracer already separates shading from scene construction and light
transport through `material_factory` and `material::scatter()`. The built-in
factory creates C++ materials, while the OSL factory creates `osl_material`
instances. Both satisfy the same interface, so the scene and recursive path
integrator remain unchanged.

The difference is contained within `scatter()`. A built-in material chooses a
scattered ray directly. An OSL material executes a shader, receives a closure
tree describing the surface response, and samples that tree to produce the ray
and attenuation expected by the integrator.

```text
scene construction
  -> material_factory
       -> builtin_materials -> C++ material
       -> osl_materials     -> osl_material + shader group

ray hit
  -> material::scatter()
       -> hit_record -> ShaderGlobals
       -> execute OSL shader group
       -> closure tree in Ci
       -> sample_closure()
  -> scattered ray + attenuation
  -> existing recursive path integrator
```

Three components form this adapter:

- [`osl_materials.h`](../src/cpu/osl/osl_materials.h) creates shader groups and
  binds scene material values.
- [`renderer.h`](../src/cpu/osl/renderer.h) and
  [`renderer.cpp`](../src/cpu/osl/renderer.cpp) manage the OSL runtime and
  execute shader groups at ray hits.
- [`shading.h`](../src/cpu/osl/shading.h),
  [`shading.cpp`](../src/cpu/osl/shading.cpp), and
  [`shaders/`](../src/cpu/osl/shaders) translate between OSL closures and the
  scattering result expected by the path tracer.

## Build and Backend Selection

`ENABLE_OSL=ON` adds OSL support to `cpu_pt`, links `OSL::oslexec`, and compiles
the `.osl` sources to `.oso` shaders. The same executable can then use `--osl`
or `--builtin`.

Both backends implement `material_factory`, so backend selection does not
affect scene construction or the integrator. The renderer loads compiled
shaders from `PT_OSL_SHADER_PATH`, and OSL diagnostics use `std::clog` to keep
standard output available for the PPM image.

## Runtime Ownership

`osl_materials` owns one shared `osl_shading_system`. Each `osl_material` holds
a reference to that system and retains the shader group for its scene material.
This fits the existing scene ownership model, in which each sphere owns a
material. 

```text
osl_materials
└── osl_shading_system
    ├── RendererServices
    ├── OSL::ShadingSystem
    └── per-thread contexts

world -> sphere -> osl_material
                    ├── system reference
                    └── ShaderGroupRef
```

Rendering still uses `tbb::parallel_for`. The shared OSL system gives each
worker its own `PerThreadInfo` and `ShadingContext`, allowing the existing
parallel renderer to execute shaders without sharing per-thread state.

## Preserving the Material Contract

The existing path tracer treats `material::scatter()` as the boundary between
materials and light transport. The OSL integration preserves that boundary
rather than exposing shader groups or closure trees to the integrator. Scene
construction still receives a `material`, and each ray hit still produces only
a scattered ray and attenuation.

All translation therefore stays inside `osl_material`: material values enter
OSL through a shader group, hit data enters through `ShaderGlobals`, and the
resulting closure tree is sampled before control returns to the integrator.
Geometry, BVH traversal, scene construction, and recursive light transport do
not need OSL-specific branches.

## Closure Adapter

OSL represents a surface response as a closure tree, but the existing material
interface returns one sampled scattering event. The closure adapter resolves
this mismatch without making the integrator aware of OSL.

The adapter exposes a small closure vocabulary that maps directly to the three
scattering behaviors already used by the path tracer:

| Closure | Result returned to the path tracer |
| ------- | ---------------------------------- |
| `diffuse(N)` | Cosine-weighted hemisphere sample |
| `rtiow_metal(N, fuzz)` | Fuzzy reflected ray |
| `rtiow_dielectric(N, eta)` | Reflected or refracted ray selected with Schlick's approximation |

The custom metal and dielectric closures preserve the behavior of the built-in
RTIOW materials, while OSL's standard diffuse closure covers the matte case.
Shaders can combine and weight these closures, but the adapter samples the tree
before returning through `material::scatter()`.

This boundary is also the extension point. Shaders can change without C++
changes while they use the registered closures. Supporting a new scattering
behavior requires adding a matching closure declaration and C++ sampler, but
does not require changing the integrator.

## Scope and Further Reading

The integration is intentionally limited to what the current path tracer can
consume: surface shaders, one layer per material group, the three closures
above, world-space identity transforms, zero screen-space derivatives, and
scalar shader execution. These are constraints of this adapter, not of OSL.

For a broader example of renderer-side OSL integration, see
[`testrender`](https://github.com/AcademySoftwareFoundation/OpenShadingLanguage/tree/main/src/testrender).
It demonstrates additional renderer services and closure evaluation that can
guide future extensions beyond the current material interface.
