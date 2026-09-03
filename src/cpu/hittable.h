#pragma once

#include "aabb.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"

#include <memory>

namespace pt
{

class material;

class hit_record
{
  public:
    point3 p;
    vec3 normal;
    std::shared_ptr<material> mat;
    double t = 0;
    bool front_face = false;

    void set_face_normal(const ray& r, const vec3& outward_normal)
    {
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable
{
  public:
    virtual ~hittable() = default;

    [[nodiscard]] virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;

    virtual aabb bounding_box() const = 0;
};

} // namespace pt
