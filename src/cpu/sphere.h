#pragma once

#include "hittable.h"
#include "material.h"

#include <cmath>
#include <memory>
#include <utility>

namespace pt
{

class sphere : public hittable
{
  public:
    sphere(const point3& center, double radius, std::shared_ptr<material> mat)
        : center(center), radius(std::fmax(0, radius)), mat(std::move(mat))
    {
        vec3 rvec = vec3(radius, radius, radius);
        bbox = aabb(center - rvec, center + rvec);
#ifdef PT_USE_OSL
        surface_params = this->mat && this->mat->needs_surface_params();
#endif
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        vec3 oc = center - r.origin();
        double a = r.direction().length_squared();
        double h = dot(r.direction(), oc);
        double c = oc.length_squared() - radius * radius;

        double discriminant = h * h - a * c;
        if (discriminant < 0)
            return false;

        double sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        double root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root))
        {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;
#ifdef PT_USE_OSL
        if (surface_params)
            set_sphere_uv(outward_normal, rec);
#endif

        return true;
    }

    aabb bounding_box() const override { return bbox; }

  private:
#ifdef PT_USE_OSL
    // Computes spherical UVs and surface tangents for a point on the sphere, given
    // as its unit outward normal. The tangents carry the sphere's radius.
    // `phi` rotates about the y axis; `theta` runs from -y to +y.
    // u = phi / (2*pi), v = theta / pi.
    void set_sphere_uv(const vec3& p, hit_record& rec) const
    {
        // `p` is unit length only to within rounding, so near either pole the
        // argument can drift past 1 and turn acos into a NaN.
        double cos_theta = std::fmin(std::fmax(-p.y(), -1.0), 1.0);

        double theta = std::acos(cos_theta);
        double phi = std::atan2(-p.z(), p.x()) + pi;

        rec.u = phi / (2 * pi);
        rec.v = theta / pi;

        double sin_theta = std::sin(theta);
        double sin_phi = std::sin(phi), cos_phi = std::cos(phi);

        rec.dpdu = 2 * pi * radius * vec3(sin_theta * sin_phi, 0, sin_theta * cos_phi);
        rec.dpdv = pi * radius * vec3(-cos_theta * cos_phi, sin_theta, cos_theta * sin_phi);
    }
#endif

    point3 center;
    double radius;
    std::shared_ptr<material> mat;
    aabb bbox;
#ifdef PT_USE_OSL
    bool surface_params = false;
#endif
};

} // namespace pt
