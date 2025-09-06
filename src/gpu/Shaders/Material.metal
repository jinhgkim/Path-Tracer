#pragma once

enum MaterialType
{
    LAMBERTIAN,
    METAL
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

struct Material
{
    MaterialType type;
    union
    {
        Lambertian lambertian;
        Metal metal;
    };
};