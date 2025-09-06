#include <chrono>
#include <cstdint>
#include <cstdlib>
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
    uint image_width;
    uint image_height;
    uint samples_per_pixel;
};

enum MaterialType
{
    LAMBERTIAN,
    METAL
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

struct Material
{
    MaterialType type;
    union
    {
        Lambertian lambertian;
        Metal metal;
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

    c.image_width = 400;
    c.image_height = int(c.image_width / aspect_ratio);
    c.image_height = (c.image_height < 1) ? 1 : c.image_height;

    int num_pixels = c.image_width * c.image_height;

    // Camera
    simd::float3 focal_length = simd::float3{0.0f, 0.0f, 1.0f};
    auto viewport_height = 2.0f;
    auto viewport_width = viewport_height * (float(c.image_width) / c.image_height);

    c.center = simd::float3{0.0f, 0.0f, 0.0f};
    auto viewport_u = simd::float3{viewport_width, 0, 0};
    auto viewport_v = simd::float3{0, -viewport_height, 0};

    c.pixel_delta_u = viewport_u / float(c.image_width);
    c.pixel_delta_v = viewport_v / float(c.image_height);

    auto viewport_upper_left = c.center - viewport_u * 0.5f - viewport_v * 0.5f - focal_length;
    c.pixel00_loc = viewport_upper_left + 0.5f * (c.pixel_delta_u + c.pixel_delta_v);

    c.samples_per_pixel = 100;

    // World
    std::vector<Sphere> world;

    Material material_ground;
    material_ground.type = LAMBERTIAN;
    material_ground.lambertian.albedo = simd::float3{0.8f, 0.8f, 0.0f};

    Material material_center;
    material_center.type = LAMBERTIAN;
    material_center.lambertian.albedo = simd::float3{0.1f, 0.2f, 0.5f};

    Material material_left;
    material_left.type = METAL;
    material_left.metal.albedo = simd::float3{0.8f, 0.8f, 0.8f};
    material_left.metal.fuzz = 0.3;

    Material material_right;
    material_right.type = METAL;
    material_right.metal.albedo = simd::float3{0.8f, 0.6f, 0.2f};
    material_right.metal.fuzz = 1.0;

    world.push_back({simd::float3{0.0f, -100.5f, -1.0f}, 100.0f, material_ground});
    world.push_back({simd::float3{0.0f, 0.0f, -1.2f}, 0.5f, material_center});
    world.push_back({simd::float3{-1.0f, 0.0f, -1.0f}, 0.5f, material_left});
    world.push_back({simd::float3{1.0f, 0.0f, -1.0f}, 0.5f, material_right});

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
    auto countBuff = device->newBuffer(&count, sizeof(Sphere), mode);

    // GPU timer starts
    auto timerStart = Clock::now();

    // Create a buffer to be sent to the command queue
    auto comandBuffer = commandQueue->commandBuffer();

    // Create an encoder to set vaulues on the compute function
    auto commandEncoder = comandBuffer->computeCommandEncoder();
    commandEncoder->setComputePipelineState(pipe);

    // Set the parameters of our gpu function
    commandEncoder->setBuffer(imageBuff, 0, 0);
    commandEncoder->setBuffer(cameraBuff, 0, 1);
    commandEncoder->setBuffer(worldBuff, 0, 2);
    commandEncoder->setBuffer(countBuff, 0, 3);

    // Figure out how many threads we need to use for our operation
    MTL::Size gridSize = MTL::Size::Make(c.image_width, c.image_height, 1);
    MTL::Size threadgroupSize = MTL::Size::Make(16, 16, 1); // 16×16 = 256
    commandEncoder->dispatchThreads(gridSize, threadgroupSize);

    // Tell the encoder that it is done encoding.  Now we can send this off to the gpu.
    commandEncoder->endEncoding();

    // Push this command to the command queue for processing
    comandBuffer->commit();

    // Wait until the gpu function completes before working with any of the data
    comandBuffer->waitUntilCompleted();

    // Get the pointer to the beginning of our data
    const simd::float3* pixels = static_cast<const simd::float3*>(imageBuff->contents());

    // Output an image
    std::cout << "P3\n" << c.image_width << " " << c.image_height << "\n255\n";
    for (int j = 0; j < c.image_height; j++)
    {
        std::clog << "\rScanlines remaining: " << (c.image_height - j) << ' ' << std::flush;
        for (int i = 0; i < c.image_width; i++)
        {
            size_t idx = j * c.image_width + i;
            simd::float3 pixel = pixels[idx];

            int ir = int(255.99 * pixel[0]);
            int ig = int(255.99 * pixel[1]);
            int ib = int(255.99 * pixel[2]);

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
    pipe->release();
    fn->release();
    lib->release();
    commandQueue->release();
    device->release();
    pool->release();
    return 0;
}