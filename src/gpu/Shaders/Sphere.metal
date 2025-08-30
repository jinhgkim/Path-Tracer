#pragma once

struct Sphere
{
    float3 center;
    float radius;

    bool hit(thread const Ray& r, float ray_tmin, float ray_tmax,
             thread HitRecord& rec) const constant
    {
        float3 oc = center - r.origin();
        float a = metal::length_squared(r.direction());
        float h = metal::dot(r.direction(), oc);
        float c = metal::length_squared(oc) - radius * radius;

        float discriminant = h * h - a * c;
        if (discriminant < 0)
            return false;

        float sqrtd = metal::sqrt(discriminant);

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