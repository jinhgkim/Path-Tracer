#pragma once

#include "vec3.h"

namespace pt
{

class ray
{
  public:
    ray() = default;

    ray(const point3& origin, const vec3& direction) : orig(origin), dir(direction) {}

    const point3& origin() const { return orig; }
    const vec3& direction() const { return dir; }

    point3 at(double t) const { return orig + t * dir; }

  private:
    point3 orig;
    vec3 dir;
};

} // namespace pt
