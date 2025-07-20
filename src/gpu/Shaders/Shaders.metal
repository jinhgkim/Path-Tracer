#include <metal_stdlib>
using namespace metal;

kernel void gradient_shader(device float3 *out           [[buffer(0)]],
                            const device uint* width     [[buffer(1)]],
                            const device uint* height    [[buffer(2)]],
                            uint2 gid [[thread_position_in_grid]])
{
    uint image_width = *width;
    uint image_height = *height;
    
    if (gid.x >= image_width || gid.y >= image_height)
        return;

    int index = gid.y * image_width + gid.x;

    float r = float(gid.x) / (image_width - 1);
    float g = float(gid.y) / (image_height - 1);
    float b = 0.0;

    out[index] = float3(r, g, b);
}