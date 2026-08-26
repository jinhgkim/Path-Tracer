#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>
#include <simd/simd.h>

using Clock = std::chrono::high_resolution_clock;

struct Camera
{
    simd::float3 pixel00_loc;
    simd::float3 pixel_delta_u;
    simd::float3 pixel_delta_v;
    simd::float3 center;
    simd::float3 defocus_disk_u;
    simd::float3 defocus_disk_v;
    uint image_width;
    uint image_height;
    uint samples_per_pixel;
    float defocus_angle;
};

inline float degrees_to_radians(float degrees)
{
    return degrees * float(M_PI) / 180.0f;
}

// Returns a random real in [0,1).
inline float random_float()
{
    // Divide in double: RAND_MAX + 1 is not representable in float.
    return float(std::rand() / (RAND_MAX + 1.0));
}

// Returns a random real in [min,max).
inline float random_float(float min, float max)
{
    return min + (max - min) * random_float();
}

inline simd::float3 random_vec3()
{
    return simd::float3{random_float(), random_float(), random_float()};
}

inline float linear_to_gamma(float linear_component)
{
    return linear_component > 0.0f ? std::sqrt(linear_component) : 0.0f;
}

inline simd::float3 random_vec3(float min, float max)
{
    return simd::float3{random_float(min, max), random_float(min, max), random_float(min, max)};
}

enum MaterialType
{
    LAMBERTIAN,
    METAL,
    DIELECTRIC
};

struct Lambertian
{
    simd::float3 albedo;
};

struct Metal
{
    simd::float3 albedo;
    float fuzz;
};

struct Dielectric
{
    float refraction_index;
};

struct Material
{
    MaterialType type;
    union
    {
        Lambertian lambertian;
        Metal metal;
        Dielectric dielectric;
    };
};

struct Sphere
{
    simd::float3 center;
    float radius;
    Material mat;
};

int main()
{
    Camera c;

    // Image
    const float aspect_ratio = 16.0 / 9.0;

    c.image_width = 1200;
    c.image_height = int(c.image_width / aspect_ratio);
    c.image_height = (c.image_height < 1) ? 1 : c.image_height;

    int num_pixels = c.image_width * c.image_height;

    // Camera
    float vfov = 20.0f; // Vertical field of view
    simd::float3 lookfrom = simd::float3{13.0f, 2.0f, 3.0f};
    simd::float3 lookat = simd::float3{0.0f, 0.0f, 0.0f};
    simd::float3 vup = simd::float3{0.0f, 1.0f, 0.0f}; // Camera-relative "up" direction

    c.defocus_angle = 0.6f;
    float focus_dist = 10.0f; // Distance from lookfrom to the plane of perfect focus

    c.center = lookfrom;

    // Determine viewport dimensions.
    float theta = degrees_to_radians(vfov);
    float h = std::tan(theta / 2.0f);
    auto viewport_height = 2.0f * h * focus_dist;
    auto viewport_width = viewport_height * (float(c.image_width) / c.image_height);

    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    auto w = simd::normalize(lookfrom - lookat);
    auto u = simd::normalize(simd::cross(vup, w));
    auto v = simd::cross(w, u);

    auto viewport_u = viewport_width * u;
    auto viewport_v = viewport_height * -v;

    c.pixel_delta_u = viewport_u / float(c.image_width);
    c.pixel_delta_v = viewport_v / float(c.image_height);

    auto viewport_upper_left = c.center - (focus_dist * w) - viewport_u * 0.5f - viewport_v * 0.5f;
    c.pixel00_loc = viewport_upper_left + 0.5f * (c.pixel_delta_u + c.pixel_delta_v);

    // Calculate the camera defocus disk basis vectors.
    auto defocus_radius = focus_dist * std::tan(degrees_to_radians(c.defocus_angle / 2.0f));
    c.defocus_disk_u = u * defocus_radius;
    c.defocus_disk_v = v * defocus_radius;

    c.samples_per_pixel = 500;

    // World
    std::vector<Sphere> world;

    auto make_lambertian = [](simd::float3 albedo)
    {
        Material m;
        m.type = LAMBERTIAN;
        m.lambertian.albedo = albedo;
        return m;
    };

    auto make_metal = [](simd::float3 albedo, float fuzz)
    {
        Material m;
        m.type = METAL;
        m.metal.albedo = albedo;
        m.metal.fuzz = fuzz;
        return m;
    };

    auto make_dielectric = [](float refraction_index)
    {
        Material m;
        m.type = DIELECTRIC;
        m.dielectric.refraction_index = refraction_index;
        return m;
    };

    world.push_back({simd::float3{0.0f, -1000.0f, 0.0f}, 1000.0f,
                     make_lambertian(simd::float3{0.5f, 0.5f, 0.5f})});

    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            auto choose_mat = random_float();
            simd::float3 center{a + 0.9f * random_float(), 0.2f, b + 0.9f * random_float()};

            if (simd::length(center - simd::float3{4.0f, 0.2f, 0.0f}) > 0.9f)
            {
                if (choose_mat < 0.8f)
                {
                    // diffuse
                    auto albedo = random_vec3() * random_vec3();
                    world.push_back({center, 0.2f, make_lambertian(albedo)});
                }
                else if (choose_mat < 0.95f)
                {
                    // metal
                    auto albedo = random_vec3(0.5f, 1.0f);
                    auto fuzz = random_float(0.0f, 0.5f);
                    world.push_back({center, 0.2f, make_metal(albedo, fuzz)});
                }
                else
                {
                    // glass
                    world.push_back({center, 0.2f, make_dielectric(1.5f)});
                }
            }
        }
    }

    world.push_back({simd::float3{0.0f, 1.0f, 0.0f}, 1.0f, make_dielectric(1.5f)});
    world.push_back(
        {simd::float3{-4.0f, 1.0f, 0.0f}, 1.0f, make_lambertian(simd::float3{0.4f, 0.2f, 0.1f})});
    world.push_back(
        {simd::float3{4.0f, 1.0f, 0.0f}, 1.0f, make_metal(simd::float3{0.7f, 0.6f, 0.5f}, 0.0f)});

    // C++ RAII
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    // The GPU we want to use
    MTL::Device* device = MTL::CreateSystemDefaultDevice();

    // A fifo queue for sending commands to the gpu
    MTL::CommandQueue* commandQueue = device->newCommandQueue();

    // A library for getting our metal functions
    auto lib = device->newDefaultLibrary();
    if (!lib)
    {
        std::cerr << "Could not load default.metallib\n";
        return 1;
    }

    // Grab our gpu function
    auto fn = lib->newFunction(NS::String::string("render", NS::UTF8StringEncoding));
    if (!fn)
    {
        std::cerr << "Kernel not found in metallib\n";
        return 1;
    }

    NS::Error* perr = nullptr;
    auto pipe = device->newComputePipelineState(fn, &perr);
    if (!pipe)
    {
        std::cerr << "Pipeline build failed: "
                  << (perr ? perr->localizedDescription()->utf8String() : "unknown") << '\n';
        return 1;
    }

    // Create the buffers to be sent to the gpu from our arrays
    const auto mode = MTL::ResourceStorageModeShared; // shared CPU-GPU memory
    auto imageBuff = device->newBuffer(num_pixels * sizeof(simd::float3), mode);
    auto cameraBuff = device->newBuffer(&c, sizeof(Camera), mode);
    auto worldBuff = device->newBuffer(world.data(), world.size() * sizeof(Sphere), mode);
    uint count = static_cast<uint>(world.size());
    auto countBuff = device->newBuffer(&count, sizeof(uint), mode);
    auto sampleRangeBuff = device->newBuffer(sizeof(simd::uint2), mode);

    // GPU timer starts
    auto timerStart = Clock::now();

    // The GPU watchdog kills long command buffers, so the render is spread over many. Sample
    // slices keep each dispatch cheap.
    const uint samples_per_dispatch = 1;

    // The kernel accumulates into this buffer
    std::memset(imageBuff->contents(), 0, num_pixels * sizeof(simd::float3));

    for (uint first = 0; first < c.samples_per_pixel; first += samples_per_dispatch)
    {
        simd::uint2 range{first, std::min(samples_per_dispatch, c.samples_per_pixel - first)};
        *static_cast<simd::uint2*>(sampleRangeBuff->contents()) = range;

        std::clog << "\rSamples remaining: " << (c.samples_per_pixel - first) << "    "
                  << std::flush;

        // Create a buffer to be sent to the command queue
        auto commandBuffer = commandQueue->commandBuffer();

        // Create an encoder to set vaulues on the compute function
        auto commandEncoder = commandBuffer->computeCommandEncoder();
        commandEncoder->setComputePipelineState(pipe);

        // Set the parameters of our gpu function
        commandEncoder->setBuffer(imageBuff, 0, 0);
        commandEncoder->setBuffer(cameraBuff, 0, 1);
        commandEncoder->setBuffer(worldBuff, 0, 2);
        commandEncoder->setBuffer(countBuff, 0, 3);
        commandEncoder->setBuffer(sampleRangeBuff, 0, 4);

        // Figure out how many threads we need to use for our operation
        MTL::Size gridSize = MTL::Size::Make(c.image_width, c.image_height, 1);
        MTL::Size threadgroupSize = MTL::Size::Make(16, 16, 1); // 16x16 = 256
        commandEncoder->dispatchThreads(gridSize, threadgroupSize);

        // Tell the encoder that it is done encoding.  Now we can send this off to the gpu.
        commandEncoder->endEncoding();

        // Push this command to the command queue for processing
        commandBuffer->commit();

        // Wait  until the gpu function completes before working with any of the data
        commandBuffer->waitUntilCompleted();

        if (auto err = commandBuffer->error())
        {
            std::cerr << "\nGPU error at sample " << first << ": "
                      << err->localizedDescription()->utf8String() << '\n';
            return 1;
        }
    }
    std::clog << "\rSamples remaining: 0    \n";

    // Get the pointer to the beginning of our data
    const simd::float3* pixels = static_cast<const simd::float3*>(imageBuff->contents());

    // Output an image
    float scale = 1.0f / c.samples_per_pixel;
    std::cout << "P3\n" << c.image_width << " " << c.image_height << "\n255\n";
    for (int j = 0; j < c.image_height; j++)
    {
        std::clog << "\rWriting scanline: " << (j + 1) << '/' << c.image_height << "    "
                  << std::flush;
        for (int i = 0; i < c.image_width; i++)
        {
            size_t idx = j * c.image_width + i;
            simd::float3 pixel = pixels[idx] * scale;

            // Apply gamma to the final pixel
            int ir = int(255.99 * linear_to_gamma(pixel[0]));
            int ig = int(255.99 * linear_to_gamma(pixel[1]));
            int ib = int(255.99 * linear_to_gamma(pixel[2]));

            std::cout << ir << " " << ig << " " << ib << "\n";
        }
    }

    // Render time
    auto timerEnd = Clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timerEnd - timerStart);

    std::clog << "Render time: " << std::chrono::duration_cast<std::chrono::hours>(ms).count()
              << "h " << std::chrono::duration_cast<std::chrono::minutes>(ms).count() % 60 << "m "
              << std::chrono::duration_cast<std::chrono::seconds>(ms).count() % 60 << "s "
              << std::endl;

    // Cleanup
    imageBuff->release();
    cameraBuff->release();
    worldBuff->release();
    countBuff->release();
    sampleRangeBuff->release();
    pipe->release();
    fn->release();
    lib->release();
    commandQueue->release();
    device->release();
    pool->release();
    return 0;
}