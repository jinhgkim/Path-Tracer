#pragma once

#include "color.h"
#include "vec3.h"

#include <OSL/genclosure.h>
#include <OSL/oslclosure.h>
#include <OSL/oslexec.h>

namespace pt
{

enum closure_id : int
{
    DIFFUSE_ID = 1,
    RTIOW_METAL_ID,
    RTIOW_DIELECTRIC_ID,
};

// OSL copies closure arguments into these structs, so registered parameters
// must match the shader declarations in order and type.
struct DiffuseParams
{
    OSL::Vec3 N;
};

struct RtiowMetalParams
{
    OSL::Vec3 N;
    float fuzz;
};

struct RtiowDielectricParams
{
    OSL::Vec3 N;
    float eta;
};

void register_closures(OSL::ShadingSystem& shadingsys);

// Flattens the shader's closure tree and samples one lobe, chosen with probability
// proportional to its mean RGB weight. `I` is the incident direction pointing toward
// the surface. On success, `wi` is the sampled direction and `weight` is the lobe's
// throughput divided by its selection probability, keeping the estimator unbiased.
[[nodiscard]] bool sample_closure(const OSL::ClosureColor* Ci, const vec3& I, vec3& wi,
                                  color& weight);

} // namespace pt
