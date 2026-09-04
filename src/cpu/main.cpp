#include "rtweekend.h"

#include "camera.h"
#include "hittable_list.h"
#include "materials.h"
#include "scene.h"

#ifdef PT_USE_OSL
#include "osl/osl_materials.h"
#endif

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace pt;

namespace
{

#ifdef PT_USE_OSL
constexpr bool osl_available = true;
#else
constexpr bool osl_available = false;
#endif

void usage(const char* program)
{
    std::clog << "usage: " << program << " [--osl | --builtin] > image.ppm\n";
}

} // namespace

int main(int argc, char** argv)
try
{
    // OSL-enabled builds default to OSL. --builtin selects the C++ materials.
    bool use_osl = osl_available;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--osl")
            use_osl = true;
        else if (arg == "--builtin")
            use_osl = false;
        else if (arg == "--help" || arg == "-h")
        {
            usage(argv[0]);
            return 0;
        }
        else
        {
            std::clog << argv[0] << ": unrecognized option '" << arg << "'\n";
            usage(argv[0]);
            return 1;
        }
    }

    if (use_osl && !osl_available)
    {
        std::clog << argv[0]
                  << ": this build has no OSL support; reconfigure with"
                     " -DBUILD_CPU_PT=ON -DENABLE_OSL=ON\n";
        return 1;
    }

    std::unique_ptr<material_factory> mats;
#ifdef PT_USE_OSL
    if (use_osl)
        mats = std::make_unique<osl_materials>(PT_OSL_SHADER_PATH);
    else
#endif
        mats = std::make_unique<builtin_materials>();

    std::clog << "Shading: " << (use_osl ? "OSL" : "built-in") << '\n';

    hittable_list world = final_scene(*mats);
    camera cam = final_camera();

    // Render
    auto start_time = std::chrono::high_resolution_clock::now();
    cam.render(world);
    auto end_time = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::clog << "Render time: " << std::chrono::duration_cast<std::chrono::hours>(ms).count()
              << 'h' << std::chrono::duration_cast<std::chrono::minutes>(ms).count() % 60 << 'm'
              << std::chrono::duration_cast<std::chrono::seconds>(ms).count() % 60 << "s\n";
}
catch (const std::exception& e)
{
    std::clog << "cpu_pt: " << e.what() << '\n';
    return 1;
}
