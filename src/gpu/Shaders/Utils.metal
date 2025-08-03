// Returns a random real in [0,1).
float random_float(thread RNG& seed)
{
    return seed.next_float();
}

// Returns a random real in [min,max).
float random_float(float min, float max, thread RNG& seed)
{
    return min + (max - min) * random_float(seed);
}

float3 random(thread RNG& seed)
{
    return float3(random_float(seed), random_float(seed), random_float(seed));
}

float3 random(float min, float max, thread RNG& seed)
{
    return float3(random_float(min, max, seed), random_float(min, max, seed),
                  random_float(min, max, seed));
}

float3 random_unit_vector(thread RNG& seed)
{
    while (true)
    {
        float3 p = random(-1.0f, 1.0f, seed);
        float lensq = metal::length_squared(p);
        if (1e-40f < lensq && lensq <= 1.0f)
            return metal::normalize(p);
    }
}

float3 random_on_hemisphere(thread const float3& normal, thread RNG& seed)
{
    float3 on_unit_sphere = random_unit_vector(seed);
    if (metal::dot(on_unit_sphere, normal) > 0.0f) // In the same hemisphere as the normal
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}

float linear_to_gamma(float linear_component)
{
    if (linear_component > 0)
        return metal::sqrt(linear_component);

    return 0;
}