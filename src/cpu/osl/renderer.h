#pragma once

#include "color.h"
#include "hittable.h"
#include "material.h"
#include "ray.h"
#include "shading.h"
#include "vec3.h"

#include <OSL/rendererservices.h>
#include <tbb/enumerable_thread_specific.h>

#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace pt
{

// The scene is world space only, so every transform is identity. Overriding the
// two forward forms is enough.
class osl_renderer_services : public OSL::RendererServices
{
  public:
    bool get_matrix(OSL::ShaderGlobals* sg, OSL::Matrix44& result, OSL::TransformationPtr xform,
                    float time) override;
    bool get_matrix(OSL::ShaderGlobals* sg, OSL::Matrix44& result,
                    OSL::TransformationPtr xform) override;
};

// A thin wrapper around ShadingSystem that also manages one ShadingContext per
// worker thread.
class osl_shading_system
{
  public:
    explicit osl_shading_system(const std::string& shader_searchpath);
    ~osl_shading_system();

    // Materials and the thread-state initializer retain addresses into this
    // object, so it cannot be copied or moved.
    osl_shading_system(const osl_shading_system&) = delete;
    osl_shading_system& operator=(const osl_shading_system&) = delete;
    osl_shading_system(osl_shading_system&&) = delete;
    osl_shading_system& operator=(osl_shading_system&&) = delete;

    [[nodiscard]] OSL::ShadingSystem& system() const { return *shadingsys; }

    // Instance parameters go between this and end_surface().
    [[nodiscard]] OSL::ShaderGroupRef begin_surface(const std::string& groupname);

    // Throws if the shader is missing or the group will not close.
    void end_surface(OSL::ShaderGroup& group, const std::string& shadername);

    // Created on first use, released when this object is destroyed.
    [[nodiscard]] OSL::ShadingContext* context();

  private:
    struct thread_state
    {
        explicit thread_state(OSL::ShadingSystem& system);
        ~thread_state();

        thread_state(const thread_state&) = delete;
        thread_state& operator=(const thread_state&) = delete;

        OSL::ShadingSystem& system;
        OSL::PerThreadInfo* info;
        OSL::ShadingContext* ctx;
    };

    std::unique_ptr<osl_renderer_services> services;
    std::unique_ptr<OSL::ErrorHandler> errhandler;

    std::unique_ptr<OSL::ShadingSystem> shadingsys;

    tbb::enumerable_thread_specific<thread_state> thread_states;
};

// Scattering from an OSL shader group: execute it, sample the closure tree it
// returns, and hand back the same pair the built-in materials do.
class osl_material : public material
{
  public:
    osl_material(osl_shading_system& sys, OSL::ShaderGroupRef group)
        : sys(sys), group(std::move(group))
    {
    }

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation,
                 ray& scattered) const override
    {
        vec3 I = unit_vector(r_in.direction());

        OSL::ShaderGlobals sg;
        globals_from_hit(sg, I, rec);

        sys.system().execute(*sys.context(), *group, sg);

        if (sg.Ci == nullptr)
            return false;

        vec3 wi;
        if (!sample_closure(sg.Ci, I, wi, attenuation))
            return false;

        scattered = ray(rec.p, wi);
        return true;
    }

    bool needs_surface_params() const override { return true; }

  private:
    osl_shading_system& sys;
    OSL::ShaderGroupRef group;

    static OSL::Vec3 to_osl(const vec3& v)
    {
        return OSL::Vec3(static_cast<float>(v.x()), static_cast<float>(v.y()),
                         static_cast<float>(v.z()));
    }

    static void globals_from_hit(OSL::ShaderGlobals& sg, const vec3& I, const hit_record& rec)
    {
        // Imath vectors start uninitialized, so `{}` would not zero this.
        std::memset(static_cast<void*>(&sg), 0, sizeof(sg));

        sg.P = to_osl(rec.p);
        sg.I = to_osl(I);

        // set_face_normal already turned the normal toward the ray, the orientation
        // OSL wants; backfacing then says whether the hit was on the inside.
        sg.N = sg.Ng = to_osl(rec.normal);
        sg.backfacing = rec.front_face ? 0 : 1;

        sg.u = static_cast<float>(rec.u);
        sg.v = static_cast<float>(rec.v);
        sg.dPdu = to_osl(rec.dpdu);
        sg.dPdv = to_osl(rec.dpdv);

        sg.surfacearea = 1; // Placeholder until light shaders are supported.

        // No mirrored transforms in this scene, so flipHandedness stays 0.

        // Screen-space differentials stay zero, so texture lookups are point sampled.
    }
};

} // namespace pt
