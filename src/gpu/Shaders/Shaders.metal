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

struct Sphere
{
    float3 center;
    float radius;
};

float hit_sphere(device const float3& center, float radius, thread const Ray& r) {
    float3 oc = center - r.origin();
    float a = length_squared(r.direction());
    float h = dot(r.direction(), oc);
    float c = length_squared(oc) - radius*radius;
    float discriminant = h*h - a*c;

    if (discriminant < 0.0f) {
        return -1.0f;
    } else {
        return (h - sqrt(discriminant)) / a;
    }
}
 
float3 ray_color(thread const Ray& r, device const Sphere& s)
{
    float t = hit_sphere(s.center, s.radius, r);
    if (t > 0.0f) {
        float3 N = normalize(r.at(t) - float3(0.0f, 0.0f, -1.0f));
        return 0.5f * float3(N.x + 1.0f, N.y + 1.0f, N.z + 1.0f);
    }

    float3 unit_direction = normalize(r.direction());
    float a = 0.5f * (unit_direction.y + 1.0f);

    return mix(float3(1.0f, 1.0f, 1.0f),
               float3(0.5f, 0.7f, 1.0f),
               a);
}

kernel void render(device float3* pixel_color      [[buffer(0)]],
                   const device Camera& c          [[buffer(1)]],
                   const device Sphere& s          [[buffer(2)]],
                   uint2 gid  [[thread_position_in_grid]])
{
    if (gid.x >= c.image_width || gid.y >= c.image_height) return;

    uint idx = gid.y * c.image_width + gid.x;

    float3 pixel_center = c.pixel00_loc + (gid.x * c.pixel_delta_u) + (gid.y * c.pixel_delta_v);
    float3 ray_direction = pixel_center - c.center;

    Ray r(c.center, ray_direction);

    pixel_color[idx] = ray_color(r, s);
}