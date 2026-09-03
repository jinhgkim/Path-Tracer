#pragma once

#include "aabb.h"
#include "hittable.h"

#include <memory>
#include <utility>
#include <vector>

namespace pt
{

class hittable_list : public hittable
{
  public:
    std::vector<std::shared_ptr<hittable>> objects;

    hittable_list() = default;
    explicit hittable_list(std::shared_ptr<hittable> object) { add(std::move(object)); }

    void clear() { objects.clear(); }

    void add(std::shared_ptr<hittable> object)
    {
        bbox = aabb(bbox, object->bounding_box());
        objects.push_back(std::move(object));
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        hit_record temp_rec;
        bool hit_anything = false;
        double closest_so_far = ray_t.max;

        for (const auto& object : objects)
        {
            if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec))
            {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    aabb bbox;
};

} // namespace pt
