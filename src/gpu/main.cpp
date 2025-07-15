#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>
#include <iostream>

int main()
{
    @autoreleasepool
    {
        using namespace MTL;

        // Grab the default GPU
        Device* device = CreateSystemDefaultDevice();
        if (!device)
        {
            std::cerr << "No Metal device available.\n";
            return 1;
        }

        std::cout << "Hello from Metal-C++!\n"
                  << "GPU  : " << device->name()->utf8String() << '\n'
                  << "Headless? " << (device->isHeadless() ? "yes" : "no") << '\n';
    }
    return 0;
}