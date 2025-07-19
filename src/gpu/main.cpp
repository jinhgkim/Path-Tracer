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

using Clock = std::chrono::high_resolution_clock;

int main()
{
    // Define image
    constexpr int image_width = 256;
    constexpr int image_height = 256;
    constexpr int num_pixels = image_width * image_height;
    constexpr int num_pixels_RGB = 3 * num_pixels;

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
    auto fn = lib->newFunction(NS::String::string("gradient_shader", NS::UTF8StringEncoding));
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
    auto imageBuff = device->newBuffer(num_pixels_RGB * sizeof(int), mode);
    auto widthBuff = device->newBuffer(&image_width, sizeof(int), mode);
    auto heightBuff = device->newBuffer(&image_height, sizeof(int), mode);

    // GPU timer starts
    auto timerStart = Clock::now();

    // Create a buffer to be sent to the command queue
    auto comandBuffer = commandQueue->commandBuffer();

    // Create an encoder to set vaulues on the compute function
    auto commandEncoder = comandBuffer->computeCommandEncoder();
    commandEncoder->setComputePipelineState(pipe);

    // Set the parameters of our gpu function
    commandEncoder->setBuffer(imageBuff, 0, 0);
    commandEncoder->setBuffer(widthBuff, 0, 1);
    commandEncoder->setBuffer(heightBuff, 0, 2);

    // Figure out how many threads we need to use for our operation
    MTL::Size gridSize = MTL::Size::Make(image_width, image_height, 1);
    MTL::Size threadgroupSize = MTL::Size::Make(16, 16, 1); // 16×16 = 256
    commandEncoder->dispatchThreads(gridSize, threadgroupSize);

    // Tell the encoder that it is done encoding.  Now we can send this off to the gpu.
    commandEncoder->endEncoding();

    // Push this command to the command queue for processing
    comandBuffer->commit();

    // Wait until the gpu function completes before working with any of the data
    comandBuffer->waitUntilCompleted();

    // Get the pointer to the beginning of our data
    const int* pixels = static_cast<const int*>(imageBuff->contents());

    // Output an image
    std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";
    for (int j = 0; j < image_height; j++)
    {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++)
        {
            size_t idx = j * image_width * 3 + i * 3;

            int ir = pixels[idx + 0];
            int ig = pixels[idx + 1];
            int ib = pixels[idx + 2];

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
    pipe->release();
    fn->release();
    lib->release();
    commandQueue->release();
    device->release();
    pool->release();
    return 0;
}