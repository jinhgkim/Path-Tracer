#include <metal_stdlib>
using namespace metal;

struct Ray
{
    float3 orig;
    float3 dir;

    Ray() {}
    Ray(float3 origin, float3 direction) : orig(origin), dir(direction) {}

    float3 origin() const { return orig; }
    float3 direction() const { return dir; }
    float3 at(float t) const { return orig + t * dir; }
};

struct Camera
{
    float3 pixel00_loc;
    float3 pixel_delta_u;
    float3 pixel_delta_v;
    float3 center;
    uint image_width;
    uint image_height;
};

float3 ray_color(thread const Ray& r)
{
    float3 unit_direction = normalize(r.direction());
    float a = 0.5f * (unit_direction.y + 1.0f);

    return mix(float3(1.0f, 1.0f, 1.0f),
               float3(0.5f, 0.7f, 1.0f),
               a);
}

kernel void render(device float3* pixel_color      [[buffer(0)]],
                   constant Camera& c              [[buffer(1)]],
                   uint2 gid  [[thread_position_in_grid]])
{
    if (gid.x >= c.image_width || gid.y >= c.image_height) return;

    uint idx = gid.y * c.image_width + gid.x;

    float3 pixel_center = c.pixel00_loc + (gid.x * c.pixel_delta_u) + (gid.y * c.pixel_delta_v);
    float3 ray_direction = pixel_center - c.center;

    Ray r(c.center, ray_direction);

    pixel_color[idx] = ray_color(r);
}