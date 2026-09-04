#pragma once

#include "color.h"
#include "material.h"
#include "materials.h"
#include "renderer.h"

#include <memory>
#include <string>

namespace pt
{

class osl_materials final : public material_factory
{
  public:
    explicit osl_materials(const std::string& shader_searchpath) : sys(shader_searchpath) {}

    std::shared_ptr<material> make_matte(const color& albedo) override
    {
        OSL::ShaderGroupRef group = sys.begin_surface("matte");

        set_param(*group, "Cs", albedo);

        sys.end_surface(*group, "matte");
        return std::make_shared<osl_material>(sys, group);
    }

    std::shared_ptr<material> make_metal(const color& albedo, double fuzz) override
    {
        OSL::ShaderGroupRef group = sys.begin_surface("metal");

        set_param(*group, "Cs", albedo);
        set_param(*group, "fuzz", static_cast<float>(fuzz));

        sys.end_surface(*group, "metal");
        return std::make_shared<osl_material>(sys, group);
    }

    std::shared_ptr<material> make_glass(double ior) override
    {
        OSL::ShaderGroupRef group = sys.begin_surface("glass");

        set_param(*group, "ior", static_cast<float>(ior));

        sys.end_surface(*group, "glass");
        return std::make_shared<osl_material>(sys, group);
    }

  private:
    // Each osl_material stores a reference to this, not a copy, so it
    // must outlive every material it creates.
    osl_shading_system sys;

    void set_param(OSL::ShaderGroup& group, const char* name, float value)
    {
        sys.system().Parameter(group, name, OSL::TypeFloat, &value);
    }

    void set_param(OSL::ShaderGroup& group, const char* name, const color& c)
    {
        OSL::Color3 value(static_cast<float>(c.x()), static_cast<float>(c.y()),
                          static_cast<float>(c.z()));
        sys.system().Parameter(group, name, OSL::TypeColor, &value);
    }
};

} // namespace pt
