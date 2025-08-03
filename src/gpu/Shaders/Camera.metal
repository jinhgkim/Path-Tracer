struct Camera
{
    float3 pixel00_loc;
    float3 pixel_delta_u;
    float3 pixel_delta_v;
    float3 center;
    uint image_width;
    uint image_height;
    uint samples_per_pixel;
};