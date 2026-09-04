#include "renderer.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace pt
{

bool osl_renderer_services::get_matrix(OSL::ShaderGlobals*, OSL::Matrix44& result,
                                       OSL::TransformationPtr, float)
{
    result.makeIdentity();
    return true;
}

bool osl_renderer_services::get_matrix(OSL::ShaderGlobals*, OSL::Matrix44& result,
                                       OSL::TransformationPtr)
{
    result.makeIdentity();
    return true;
}

namespace
{

// OSL diagnostics must not land on stdout since that is where the output PPM goes.
class clog_error_handler : public OSL::ErrorHandler
{
  public:
    void operator()(int errcode, const std::string& msg) override
    {
        const char* prefix = "OSL: ";
        switch (errcode & 0xffff0000)
        {
        case EH_ERROR:
        case EH_SEVERE:
            prefix = "OSL error: ";
            break;
        case EH_WARNING:
            prefix = "OSL warning: ";
            break;
        }

        std::clog << prefix << msg << '\n';
    }
};

} // namespace

osl_shading_system::osl_shading_system(const std::string& shader_searchpath)
    : services(std::make_unique<osl_renderer_services>()),
      errhandler(std::make_unique<clog_error_handler>()),
      shadingsys(std::make_unique<OSL::ShadingSystem>(services.get(), nullptr, errhandler.get())),
      thread_states([this] { return thread_state(*shadingsys); })
{
    shadingsys->attribute("searchpath:shader", shader_searchpath);

    register_closures(*shadingsys);
}

osl_shading_system::~osl_shading_system() = default;

osl_shading_system::thread_state::thread_state(OSL::ShadingSystem& system)
    : system(system), info(system.create_thread_info()), ctx(system.get_context(info))
{
}

osl_shading_system::thread_state::~thread_state()
{
    system.release_context(ctx);
    system.destroy_thread_info(info);
}

OSL::ShaderGroupRef osl_shading_system::begin_surface(const std::string& groupname)
{
    return shadingsys->ShaderGroupBegin(groupname);
}

void osl_shading_system::end_surface(OSL::ShaderGroup& group, const std::string& shadername)
{
    const bool loaded = shadingsys->Shader(group, "surface", shadername, "layer");

    // Finalize the group even if shader loading failed.
    const bool closed = shadingsys->ShaderGroupEnd(group);

    if (!loaded)
        throw std::runtime_error("could not load OSL shader '" + shadername + "'; is " +
                                 shadername + ".oso on the shader search path?");

    if (!closed)
        throw std::runtime_error("could not close the shader group for '" + shadername + "'");
}

OSL::ShadingContext* osl_shading_system::context()
{
    return thread_states.local().ctx;
}

} // namespace pt
