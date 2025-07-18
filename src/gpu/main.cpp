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

constexpr std::size_t count = 3'000'000;

std::vector<float> randomArray(std::size_t n = count)
{
    std::vector<float> v(n);
    for (std::size_t i = 0; i < n; i++)
        v[i] = (std::rand() / float(RAND_MAX)) * 10.0f;
    return v;
}

using Clock = std::chrono::high_resolution_clock;

int main()
{
    // Create random arrays
    std::vector<float> arr1 = randomArray();
    std::vector<float> arr2 = randomArray();

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
    auto fn =
        lib->newFunction(NS::String::string("addition_compute_function", NS::UTF8StringEncoding));
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
    auto arr1Buff = device->newBuffer(arr1.data(), arr1.size() * sizeof(float), mode);
    auto arr2Buff = device->newBuffer(arr2.data(), arr2.size() * sizeof(float), mode);
    auto resultBuff = device->newBuffer(arr1.size() * sizeof(float), mode);

    // GPU timer starts
    auto gpuStart = Clock::now();

    // Create a buffer to be sent to the command queue
    auto comandBuffer = commandQueue->commandBuffer();

    // Create an encoder to set vaulues on the compute function
    auto commandEncoder = comandBuffer->computeCommandEncoder();
    commandEncoder->setComputePipelineState(pipe);

    // Set the parameters of our gpu function
    commandEncoder->setBuffer(arr1Buff, 0, 0);
    commandEncoder->setBuffer(arr2Buff, 0, 1);
    commandEncoder->setBuffer(resultBuff, 0, 2);

    // Figure out how many threads we need to use for our operatio
    std::size_t maxThreadsPerThreadGroup = pipe->maxTotalThreadsPerThreadgroup(); // 1024
    commandEncoder->dispatchThreads({count, 1, 1}, {maxThreadsPerThreadGroup, 1, 1});

    // Tell the encoder that it is done encoding.  Now we can send this off to the gpu.
    commandEncoder->endEncoding();

    // Push this command to the command queue for processing
    comandBuffer->commit();

    // Wait until the gpu function completes before working with any of the data
    comandBuffer->waitUntilCompleted();

    double gpuTime = std::chrono::duration<double>(Clock::now() - gpuStart).count();

    // Get the pointer to the beginning of our data
    const float* gpuOut = static_cast<const float*>(resultBuff->contents());

    std::cout << "\nGPU\n";
    for (int i = 0; i < 3; i++)
        std::cout << arr1[i] << " + " << arr2[i] << " = " << gpuOut[i] << '\n';
    std::cout << "Time elapsed  " << std::fixed << std::setprecision(5) << gpuTime << " s\n\n";

    // CPU timer starts
    auto cpuStart = Clock::now();

    std::vector<float> cpuOut(count);
    for (std::size_t i = 0; i < count; i++)
        cpuOut[i] = arr1[i] + arr2[i];

    double cpuTime = std::chrono::duration<double>(Clock::now() - cpuStart).count();

    std::cout << "CPU\n";
    for (int i = 0; i < 3; i++)
        std::cout << arr1[i] << " + " << arr2[i] << " = " << cpuOut[i] << '\n';
    std::cout << "Time elapsed    " << std::fixed << std::setprecision(5) << cpuTime << " s\n";

    // Cleanup
    resultBuff->release();
    arr2Buff->release();
    arr1Buff->release();
    pipe->release();
    fn->release();
    lib->release();
    commandQueue->release();
    device->release();
    pool->release();
    return 0;
}

/* Example output:

GPU
5.392 + 0.831621 = 6.22362
7.85796 + 5.35374 = 13.2117
4.27459 + 6.07924 = 10.3538
Time elapsed  0.00439 s

CPU
5.39200 + 0.83162 = 6.22362
7.85796 + 5.35374 = 13.21170
4.27459 + 6.07924 = 10.35383
Time elapsed    0.03291 s

*/