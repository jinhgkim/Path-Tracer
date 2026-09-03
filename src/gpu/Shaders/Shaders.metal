#include <metal_stdlib>
#include "RNG.metal"
#include "Utils.metal"
#include "Ray.metal"
#include "Material.metal"
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
    float3 curr_attenuation = float3(1.0f, 1.0f, 1.0f);

    for (int i = 0; i < 50; i++)
    {
        HitRecord rec;
        if (hit(world, count, curr_ray, 0.001f, INFINITY, rec))
        {
            if (rec.mat.type == MaterialType::LAMBERTIAN)
            {
                float3 scatter_direction = rec.normal + random_unit_vector(seed);
                curr_ray = Ray(rec.p, scatter_direction);
                curr_attenuation *= rec.mat.lambertian.albedo;
            }
            else if (rec.mat.type == MaterialType::METAL)
            {
                float fuzz = rec.mat.metal.fuzz < 1.0f ? rec.mat.metal.fuzz : 1.0f;
                float3 reflected = metal::reflect(curr_ray.direction(), rec.normal);
                reflected = normalize(reflected) + (fuzz * random_unit_vector(seed));
                curr_ray = Ray(rec.p, reflected);

                if (metal::dot(curr_ray.direction(), rec.normal) > 0)
                    curr_attenuation *= rec.mat.metal.albedo;
            }
            else if (rec.mat.type == MaterialType::DIELECTRIC)
            {
                float ri = rec.front_face ? (1.0f / rec.mat.dielectric.refraction_index)
                                          : rec.mat.dielectric.refraction_index;

                float3 unit_direction = normalize(curr_ray.direction());
                float cos_theta = metal::fmin(dot(-unit_direction, rec.normal), 1.0f);
                float sin_theta = metal::sqrt(1.0f - cos_theta * cos_theta);

                bool cannot_refract = ri * sin_theta > 1.0f;
                float3 direction;

                if (cannot_refract || Dielectric::reflectance(cos_theta, ri) > random_float(seed))
                    direction = metal::reflect(unit_direction, rec.normal);
                else
                    direction = metal::refract(unit_direction, rec.normal, ri);

                curr_ray = Ray(rec.p, direction);
            }
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
                   constant uint2& sample_range [[buffer(4)]],
                   uint2 gid        [[thread_position_in_grid]])
{
    if (gid.x >= c.image_width || gid.y >= c.image_height)
        return;

    uint idx = gid.y * c.image_width + gid.x;

    RNG seed;
    seed.init(idx * 0x9e3779b9u + sample_range.x * 0x85ebca6bu);

    float3 color_acc(0.0f, 0.0f, 0.0f);

    for (uint s = 0; s < sample_range.y; s++)
    {
        Ray r = c.get_ray(gid.x, gid.y, seed);
        color_acc += ray_color(r, world, count, seed);
    }

    pixel_color[idx] += color_acc;
}