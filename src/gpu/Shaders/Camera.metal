#pragma once

struct Camera
{
    float3 pixel00_loc;
    float3 pixel_delta_u;
    float3 pixel_delta_v;
    float3 center;
    float3 defocus_disk_u;
    float3 defocus_disk_v;
    uint image_width;
    uint image_height;
    uint samples_per_pixel;
    float defocus_angle;

    // Construct a camera ray originating from the defocus disk and directed at a randomly
    // sampled point around the pixel location i, j.
    Ray get_ray(uint i, uint j, thread RNG& seed) const constant
    {
        float2 offset = float2(random_float(seed) - 0.5f, random_float(seed) - 0.5f);
        float3 pixel_sample =
            pixel00_loc + ((i + offset.x) * pixel_delta_u) + ((j + offset.y) * pixel_delta_v);

        float3 ray_origin = (defocus_angle <= 0.0f) ? center : defocus_disk_sample(seed);
        float3 ray_direction = pixel_sample - ray_origin;

        return Ray(ray_origin, ray_direction);
    }

    // Returns a random point in the camera defocus disk.
    float3 defocus_disk_sample(thread RNG& seed) const constant
    {
        float3 p = random_in_unit_disk(seed);
        return center + (p.x * defocus_disk_u) + (p.y * defocus_disk_v);
    }
};
