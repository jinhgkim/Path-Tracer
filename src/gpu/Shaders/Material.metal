#pragma once

enum MaterialType
{
    LAMBERTIAN,
    METAL,
    DIELECTRIC
};

struct Lambertian
{
    float3 albedo;
};

struct Metal
{
    float3 albedo;
    float fuzz;
};

struct Dielectric
{
    float refraction_index;

    static float reflectance(float cosine, float refraction_index)
    {
        // Use Schlick's approximation for reflectance.
        float r0 = (1.0f - refraction_index) / (1.0f + refraction_index);
        r0 = r0 * r0;
        return r0 + (1.0f - r0) * metal::pow((1.0f - cosine), 5.0f);
    }
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