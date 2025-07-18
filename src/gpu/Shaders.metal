#include <metal_stdlib>
using namespace metal;

kernel void addition_compute_function(const device float *arr1 [[buffer(0)]],
                                      const device float *arr2 [[buffer(1)]],
                                      device float       *out  [[buffer(2)]],
                                      uint gid [[thread_position_in_grid]])
{
    out[gid] = arr1[gid] + arr2[gid];
}