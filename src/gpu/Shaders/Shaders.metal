#include <metal_stdlib>
#include "RNG.metal"
#include "Utils.metal"
#include "Ray.metal"
#include "Hittable.metal"
#include "Sphere.metal"
#include "Camera.metal"

using namespace metal;

bool hit(constant Sphere* world, constant uint& count, thread const Ray& r, float ray_tmin,
         float ray_tmax, thread HitRecord& rec)
{
    HitRecord temp_rec;
    bool hit_anything = false;
    float closest_so_far = ray_tmax;

    for (uint i = 0; i < count; i++)
    {
        if (world[i].hit(r, ray_tmin, closest_so_far, temp_rec))
        {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}

float3 ray_color(thread const Ray& r, constant Sphere* world, constant uint& count,
                 thread RNG& seed)
{
    Ray curr_ray = r;
    float curr_attenuation = 1.0f;

    for (int i = 0; i < 50; i++)
    {

        HitRecord rec;
        if (hit(world, count, curr_ray, 0.001f, INFINITY, rec))
        {
            float3 direction = rec.normal + random_unit_vector(seed);
            curr_attenuation *= 0.5f;
            curr_ray = Ray(rec.p, direction);
        }
        else
        {
            float3 unit_direction = normalize(curr_ray.direction());
            float a = 0.5f * (unit_direction.y + 1.0f);
            return curr_attenuation * mix(float3(1.0f, 1.0f, 1.0f), float3(0.5f, 0.7f, 1.0f), a);
        }
    }
    return float3(0.0f, 0.0f, 0.0f);
}

kernel void render(device float3* pixel_color   [[buffer(0)]],
                   constant Camera& c           [[buffer(1)]],
                   constant Sphere* world       [[buffer(2)]],
                   constant uint& count         [[buffer(3)]],
                   uint2 gid        [[thread_position_in_grid]])
{
    if (gid.x >= c.image_width || gid.y >= c.image_height)
        return;

    uint idx = gid.y * c.image_width + gid.x;

    float3 color_acc(0.0f, 0.0f, 0.0f);

    RNG seed;
    seed.init(idx);

    for (uint s = 0; s < c.samples_per_pixel; s++)
    {
        float3 offset = float3(random_float(seed) - 0.5f, random_float(seed) - 0.5f, 0.0f);
        float3 pixel_sample = c.pixel00_loc + ((gid.x + offset.x) * c.pixel_delta_u) +
                              ((gid.y + offset.y) * c.pixel_delta_v);
        float3 ray_direction = pixel_sample - c.center;

        Ray r(c.center, ray_direction);
        color_acc += ray_color(r, world, count, seed);
    }
    pixel_color[idx] = float3(linear_to_gamma(color_acc.x / c.samples_per_pixel),
                              linear_to_gamma(color_acc.y / c.samples_per_pixel),
                              linear_to_gamma(color_acc.z / c.samples_per_pixel));
}