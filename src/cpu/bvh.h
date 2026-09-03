#pragma once

#include "aabb.h"
#include "hittable.h"
#include "hittable_list.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

namespace pt
{

class bvh_node : public hittable
{
  public:
    explicit bvh_node(hittable_list& list) : bvh_node(list.objects, 0, list.objects.size()) {}

    bvh_node(std::vector<std::shared_ptr<hittable>>& objects, std::size_t start, std::size_t end)
    {
        // Build the bounding box of the span of source objects.
        bbox = aabb::empty;
        for (std::size_t i = start; i < end; i++)
        {
            bbox = aabb(objects[i]->bounding_box(), bbox);
        }

        // Pick an axis to split on
        int axis = bbox.longest_axis();

        // Sort the primitives
        auto comparator = (axis == 0) ? box_x_compare : (axis == 1) ? box_y_compare : box_z_compare;

        std::size_t object_span = end - start;

        if (object_span == 1)
        {
            left = objects[start];
            right = objects[start];
        }
        else if (object_span == 2)
        {
            left = objects[start];
            right = objects[start + 1];
        }
        else
        {
            // Put half in each subtree
            std::sort(objects.begin() + start, objects.begin() + end, comparator);

            std::size_t mid = start + object_span / 2;
            left = std::make_shared<bvh_node>(objects, start, mid);
            right = std::make_shared<bvh_node>(objects, mid, end);
        }
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override
    {
        if (!bbox.hit(r, ray_t))
            return false;

        bool hit_left = left->hit(r, ray_t, rec);
        bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

        return hit_left || hit_right;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    std::shared_ptr<hittable> left;
    std::shared_ptr<hittable> right;
    aabb bbox;

    static bool box_compare(const std::shared_ptr<hittable>& a, const std::shared_ptr<hittable>& b,
                            int axis_index)
    {
        interval a_axis_interval = a->bounding_box().axis_interval(axis_index);
        interval b_axis_interval = b->bounding_box().axis_interval(axis_index);
        return a_axis_interval.min < b_axis_interval.min;
    }

    static bool box_x_compare(const std::shared_ptr<hittable>& a,
                              const std::shared_ptr<hittable>& b)
    {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare(const std::shared_ptr<hittable>& a,
                              const std::shared_ptr<hittable>& b)
    {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare(const std::shared_ptr<hittable>& a,
                              const std::shared_ptr<hittable>& b)
    {
        return box_compare(a, b, 2);
    }
};

} // namespace pt
