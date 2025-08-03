#include <metal_stdlib>
using namespace metal;

struct RNG
{
    uint state;

    void init(uint x) { state = x; }

    uint next_uint()
    {
        state ^= state >> 17;
        state *= 0xed5ad4bb;
        state ^= state >> 11;
        state *= 0xac4c1b51;
        state ^= state >> 15;
        state *= 0x31848bab;
        state ^= state >> 14;
        return state;
    }

    float next_float()
    {
        return float(next_uint()) / 4294967296.0; // [0, 1)
    }
};

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
    uint samples_per_pixel;
};

struct HitRecord
{
    float3 p;
    float3 normal;
    float t;
    bool front_face;

    void set_face_normal(thread const Ray& r, thread const float3& outward_normal)
    {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

struct Sphere
{
    float3 center;
    float radius;

    bool hit(thread const Ray& r, float ray_tmin, float ray_tmax,
             thread HitRecord& rec) const constant
    {
        float3 oc = center - r.origin();
        float a = length_squared(r.direction());
        float h = dot(r.direction(), oc);
        float c = length_squared(oc) - radius * radius;

        float discriminant = h * h - a * c;
        if (discriminant < 0)
            return false;

        float sqrtd = sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        float root = (h - sqrtd) / a;
        if (root <= ray_tmin || ray_tmax <= root)
        {
            root = (h + sqrtd) / a;
            if (root <= ray_tmin || ray_tmax <= root)
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);

        float3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);

        return true;
    }
};

// Returns a random real in [0,1).
float random_float(thread RNG& seed)
{
    return seed.next_float();
}

// Returns a random real in [min,max).
float random_float(float min, float max, thread RNG& seed)
{
    return min + (max - min) * random_float(seed);
}

float3 random(thread RNG& seed)
{
    return float3(random_float(seed), random_float(seed), random_float(seed));
}

float3 random(float min, float max, thread RNG& seed)
{
    return float3(random_float(min, max, seed), random_float(min, max, seed),
                  random_float(min, max, seed));
}

float3 random_unit_vector(thread RNG& seed)
{
    while (true)
    {
        float3 p = random(-1.0f, 1.0f, seed);
        float lensq = length_squared(p);
        if (1e-40f < lensq && lensq <= 1.0f)
            return normalize(p);
    }
}

float3 random_on_hemisphere(thread const float3& normal, thread RNG& seed)
{
    float3 on_unit_sphere = random_unit_vector(seed);
    if (dot(on_unit_sphere, normal) > 0.0f) // In the same hemisphere as the normal
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}

float linear_to_gamma(float linear_component)
{
    if (linear_component > 0)
        return sqrt(linear_component);

    return 0;
}

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