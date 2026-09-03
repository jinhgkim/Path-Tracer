#pragma once

#include <cmath>
#include <limits>
#include <random>

namespace pt
{

// Constants

inline constexpr double infinity = std::numeric_limits<double>::infinity();
inline constexpr double pi = 3.1415926535897932385;

// Utility Functions

inline double degrees_to_radians(double degrees)
{
    return degrees * pi / 180.0;
}

inline std::mt19937& random_generator()
{
    // Each rendering thread gets an independent generator.
    thread_local std::mt19937 generator;
    return generator;
}

inline void seed_random_generator(unsigned int seed)
{
    random_generator().seed(seed);
}

// Returns a random real in [0,1).
inline double random_double()
{
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(random_generator());
}

// Returns a random real in [min,max).
inline double random_double(double min, double max)
{
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(random_generator());
}

// Returns a random integer in [min,max].
inline int random_int(int min, int max)
{
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(random_generator());
}

} // namespace pt
