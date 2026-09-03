#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <simd/simd.h>

using Clock = std::chrono::high_resolution_clock;

inline constexpr float PI = 3.1415926535897932385f;

struct Camera
{
    simd::float3 pixel00_loc;
    simd::float3 pixel_delta_u;
    simd::float3 pixel_delta_v;
    simd::float3 center;
    simd::float3 defocus_disk_u;
    simd::float3 defocus_disk_v;
    std::uint32_t image_width;
    std::uint32_t image_height;
    std::uint32_t samples_per_pixel;
    float defocus_angle;
};

inline float degrees_to_radians(float degrees)
{
    return degrees * PI / 180.0f;
}

// Returns a random real in [0,1).
inline float random_float()
{
    // Divide in double: RAND_MAX + 1 is not representable in float.
    return static_cast<float>(std::rand() / (RAND_MAX + 1.0));
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

enum class MaterialType : std::uint32_t
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
    // Seed used to lay out the random spheres below; change it for a different scene.
    std::srand(2);

    Camera c{};

    // Image
    const float aspect_ratio = 16.0f / 9.0f;

    c.image_width = 1200;
    c.image_height = static_cast<std::uint32_t>(c.image_width / aspect_ratio);
    c.image_height = (c.image_height < 1) ? 1 : c.image_height;

    std::size_t num_pixels = static_cast<std::size_t>(c.image_width) * c.image_height;

    // Camera
    const float vfov = 20.0f; // Vertical field of view
    const simd::float3 lookfrom{13.0f, 2.0f, 3.0f};
    const simd::float3 lookat{0.0f, 0.0f, 0.0f};
    const simd::float3 vup{0.0f, 1.0f, 0.0f}; // Camera-relative "up" direction

    c.defocus_angle = 0.6f;
    const float focus_dist = 10.0f; // Distance from lookfrom to the plane of perfect focus

    c.center = lookfrom;

    // Determine viewport dimensions.
    float theta = degrees_to_radians(vfov);
    float h = std::tan(theta / 2.0f);
    float viewport_height = 2.0f * h * focus_dist;
    float viewport_width = viewport_height * (static_cast<float>(c.image_width) / c.image_height);

    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    simd::float3 w = simd::normalize(lookfrom - lookat);
    simd::float3 u = simd::normalize(simd::cross(vup, w));
    simd::float3 v = simd::cross(w, u);

    simd::float3 viewport_u = viewport_width * u;
    simd::float3 viewport_v = viewport_height * -v;

    c.pixel_delta_u = viewport_u / static_cast<float>(c.image_width);
    c.pixel_delta_v = viewport_v / static_cast<float>(c.image_height);

    simd::float3 viewport_upper_left =
        c.center - (focus_dist * w) - viewport_u * 0.5f - viewport_v * 0.5f;
    c.pixel00_loc = viewport_upper_left + 0.5f * (c.pixel_delta_u + c.pixel_delta_v);

    // Calculate the camera defocus disk basis vectors.
    float defocus_radius = focus_dist * std::tan(degrees_to_radians(c.defocus_angle / 2.0f));
    c.defocus_disk_u = u * defocus_radius;
    c.defocus_disk_v = v * defocus_radius;

    c.samples_per_pixel = 500;

    // World
    std::vector<Sphere> world;

    auto make_lambertian = [](simd::float3 albedo)
    {
        Material m{};
        m.type = MaterialType::LAMBERTIAN;
        m.lambertian.albedo = albedo;
        return m;
    };

    auto make_metal = [](simd::float3 albedo, float fuzz)
    {
        Material m{};
        m.type = MaterialType::METAL;
        m.metal.albedo = albedo;
        m.metal.fuzz = fuzz;
        return m;
    };

    auto make_dielectric = [](float refraction_index)
    {
        Material m{};
        m.type = MaterialType::DIELECTRIC;
        m.dielectric.refraction_index = refraction_index;
        return m;
    };

    world.push_back({simd::float3{0.0f, -1000.0f, 0.0f}, 1000.0f,
                     make_lambertian(simd::float3{0.5f, 0.5f, 0.5f})});

    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            float choose_mat = random_float();
            simd::float3 center{a + 0.9f * random_float(), 0.2f, b + 0.9f * random_float()};

            if (simd::length(center - simd::float3{4.0f, 0.2f, 0.0f}) > 0.9f)
            {
                if (choose_mat < 0.8f)
                {
                    // diffuse
                    simd::float3 albedo = random_vec3() * random_vec3();
                    world.push_back({center, 0.2f, make_lambertian(albedo)});
                }
                else if (choose_mat < 0.95f)
                {
                    // metal
                    simd::float3 albedo = random_vec3(0.5f, 1.0f);
                    float fuzz = random_float(0.0f, 0.5f);
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
    NS::SharedPtr<NS::AutoreleasePool> pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

    // The GPU we want to use
    NS::SharedPtr<MTL::Device> device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    if (!device)
    {
        std::cerr << "No Metal device available\n";
        return 1;
    }

    // A FIFO queue for sending commands to the GPU
    NS::SharedPtr<MTL::CommandQueue> commandQueue = NS::TransferPtr(device->newCommandQueue());

    // A library for getting our Metal functions
    NS::SharedPtr<MTL::Library> lib = NS::TransferPtr(device->newDefaultLibrary());
    if (!lib)
    {
        std::cerr << "Could not load default.metallib\n";
        return 1;
    }

    // Grab our GPU function
    NS::SharedPtr<MTL::Function> fn =
        NS::TransferPtr(lib->newFunction(NS::String::string("render", NS::UTF8StringEncoding)));
    if (!fn)
    {
        std::cerr << "Kernel not found in metallib\n";
        return 1;
    }

    NS::Error* perr = nullptr;
    NS::SharedPtr<MTL::ComputePipelineState> pipe =
        NS::TransferPtr(device->newComputePipelineState(fn.get(), &perr));
    if (!pipe)
    {
        std::cerr << "Pipeline build failed: "
                  << (perr ? perr->localizedDescription()->utf8String() : "unknown") << '\n';
        return 1;
    }

    // Create the buffers to be sent to the GPU from our arrays
    const MTL::ResourceOptions mode = MTL::ResourceStorageModeShared; // shared CPU-GPU memory
    NS::SharedPtr<MTL::Buffer> imageBuff =
        NS::TransferPtr(device->newBuffer(num_pixels * sizeof(simd::float3), mode));
    NS::SharedPtr<MTL::Buffer> cameraBuff =
        NS::TransferPtr(device->newBuffer(&c, sizeof(Camera), mode));
    NS::SharedPtr<MTL::Buffer> worldBuff =
        NS::TransferPtr(device->newBuffer(world.data(), world.size() * sizeof(Sphere), mode));
    const std::uint32_t count = static_cast<std::uint32_t>(world.size());
    NS::SharedPtr<MTL::Buffer> countBuff =
        NS::TransferPtr(device->newBuffer(&count, sizeof(count), mode));
    NS::SharedPtr<MTL::Buffer> sampleRangeBuff =
        NS::TransferPtr(device->newBuffer(sizeof(simd::uint2), mode));

    // GPU timer starts
    Clock::time_point timerStart = Clock::now();

    // The GPU watchdog kills long command buffers, so the render is spread over many. Sample
    // slices keep each dispatch cheap.
    const std::uint32_t samples_per_dispatch = 1;

    // The kernel accumulates into this buffer
    std::memset(imageBuff->contents(), 0, num_pixels * sizeof(simd::float3));

    for (std::uint32_t first = 0; first < c.samples_per_pixel; first += samples_per_dispatch)
    {
        simd::uint2 range{first, std::min(samples_per_dispatch, c.samples_per_pixel - first)};
        *static_cast<simd::uint2*>(sampleRangeBuff->contents()) = range;

        std::clog << "\rSamples remaining: " << (c.samples_per_pixel - first) << "    "
                  << std::flush;

        // Create a buffer to be sent to the command queue
        MTL::CommandBuffer* commandBuffer = commandQueue->commandBuffer();

        // Create an encoder to set values on the compute function
        MTL::ComputeCommandEncoder* commandEncoder = commandBuffer->computeCommandEncoder();
        commandEncoder->setComputePipelineState(pipe.get());

        // Set the parameters of our GPU function
        commandEncoder->setBuffer(imageBuff.get(), 0, 0);
        commandEncoder->setBuffer(cameraBuff.get(), 0, 1);
        commandEncoder->setBuffer(worldBuff.get(), 0, 2);
        commandEncoder->setBuffer(countBuff.get(), 0, 3);
        commandEncoder->setBuffer(sampleRangeBuff.get(), 0, 4);

        // Figure out how many threads we need to use for our operation
        MTL::Size gridSize = MTL::Size::Make(c.image_width, c.image_height, 1);
        MTL::Size threadgroupSize = MTL::Size::Make(16, 16, 1); // 16x16 = 256
        commandEncoder->dispatchThreads(gridSize, threadgroupSize);

        // Tell the encoder that it is done encoding. Now we can send this off to the GPU.
        commandEncoder->endEncoding();

        // Push this command to the command queue for processing
        commandBuffer->commit();

        // Wait until the GPU function completes before working with any of the data
        commandBuffer->waitUntilCompleted();

        if (NS::Error* err = commandBuffer->error())
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
    for (std::uint32_t j = 0; j < c.image_height; j++)
    {
        std::clog << "\rWriting scanline: " << (j + 1) << '/' << c.image_height << "    "
                  << std::flush;
        for (std::uint32_t i = 0; i < c.image_width; i++)
        {
            std::size_t idx = static_cast<std::size_t>(j) * c.image_width + i;
            simd::float3 pixel = pixels[idx] * scale;

            // Apply gamma to the final pixel
            int ir = static_cast<int>(255.99f * linear_to_gamma(pixel[0]));
            int ig = static_cast<int>(255.99f * linear_to_gamma(pixel[1]));
            int ib = static_cast<int>(255.99f * linear_to_gamma(pixel[2]));

            std::cout << ir << " " << ig << " " << ib << "\n";
        }
    }

    // Render time
    Clock::time_point timerEnd = Clock::now();
    std::chrono::milliseconds ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(timerEnd - timerStart);

    std::clog << "Render time: " << std::chrono::duration_cast<std::chrono::hours>(ms).count()
              << "h " << std::chrono::duration_cast<std::chrono::minutes>(ms).count() % 60 << "m "
              << std::chrono::duration_cast<std::chrono::seconds>(ms).count() % 60 << "s\n";

    return 0;
}
