#pragma once

#include "color.h"
#include "material.h"

#include <memory>

namespace pt
{

class material_factory
{
  public:
    virtual ~material_factory() = default;

    material_factory() = default;
    material_factory(const material_factory&) = delete;
    material_factory& operator=(const material_factory&) = delete;

    virtual std::shared_ptr<material> make_matte(const color& albedo) = 0;
    virtual std::shared_ptr<material> make_metal(const color& albedo, double fuzz) = 0;
    virtual std::shared_ptr<material> make_glass(double ior) = 0;
};

class builtin_materials final : public material_factory
{
  public:
    std::shared_ptr<material> make_matte(const color& albedo) override
    {
        return std::make_shared<lambertian>(albedo);
    }

    std::shared_ptr<material> make_metal(const color& albedo, double fuzz) override
    {
        return std::make_shared<metal>(albedo, fuzz);
    }

    std::shared_ptr<material> make_glass(double ior) override
    {
        return std::make_shared<dielectric>(ior);
    }
};

} // namespace pt
