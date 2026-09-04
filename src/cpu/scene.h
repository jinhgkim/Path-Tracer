#pragma once

#include "bvh.h"
#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "material.h"
#include "materials.h"
#include "rtweekend.h"
#include "sphere.h"
#include "vec3.h"

#include <memory>

namespace pt
{

// The Ray Tracing in One Weekend cover scene.
inline hittable_list final_scene(material_factory& mats)
{
    // Seed used to lay out the random spheres below; change it for a different scene.
    seed_random_generator(2);

    hittable_list world;

    auto ground_material = mats.make_matte(color(0.5, 0.5, 0.5));
    world.add(std::make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            double choose_mat = random_double();
            point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9)
            {
                std::shared_ptr<material> sphere_material;

                if (choose_mat < 0.8)
                {
                    // diffuse
                    color albedo = color::random() * color::random();
                    sphere_material = mats.make_matte(albedo);
                }
                else if (choose_mat < 0.95)
                {
                    // metal
                    color albedo = color::random(0.5, 1);
                    double fuzz = random_double(0, 0.5);
                    sphere_material = mats.make_metal(albedo, fuzz);
                }
                else
                {
                    // glass
                    sphere_material = mats.make_glass(1.5);
                }

                world.add(std::make_shared<sphere>(center, 0.2, sphere_material));
            }
        }
    }

    auto material1 = mats.make_glass(1.5);
    world.add(std::make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = mats.make_matte(color(0.4, 0.2, 0.1));
    world.add(std::make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = mats.make_metal(color(0.7, 0.6, 0.5), 0.0);
    world.add(std::make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return hittable_list(std::make_shared<bvh_node>(world));
}

// The camera the scene above is framed for.
inline camera final_camera()
{
    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 1200;
    cam.samples_per_pixel = 500;
    cam.max_depth = 50;

    cam.vfov = 20;
    cam.lookfrom = point3(13, 2, 3);
    cam.lookat = point3(0, 0, 0);
    cam.vup = vec3(0, 1, 0);

    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;

    return cam;
}

} // namespace pt
