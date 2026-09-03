#pragma once

#include "rtweekend.h"

#include <algorithm>

namespace pt
{

class interval
{
  public:
    double min, max;

    interval() : min(+infinity), max(-infinity) {} // Default interval is empty

    interval(double min, double max) : min(min), max(max) {}

    // Create the interval tightly enclosing the two input intervals.
    interval(const interval& a, const interval& b)
        : min(a.min <= b.min ? a.min : b.min), max(a.max >= b.max ? a.max : b.max)
    {
    }

    double size() const { return max - min; }

    bool contains(double x) const { return min <= x && x <= max; }

    bool surrounds(double x) const { return min < x && x < max; }

    double clamp(double x) const { return std::clamp(x, min, max); }

    interval expand(double delta) const
    {
        double padding = delta / 2;
        return interval(min - padding, max + padding);
    }

    static const interval empty, universe;
};

inline const interval interval::empty = interval(+infinity, -infinity);
inline const interval interval::universe = interval(-infinity, +infinity);

} // namespace pt
