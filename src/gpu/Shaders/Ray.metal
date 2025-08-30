#pragma once

struct Ray
{
    float3 orig;
    float3 dir;

    Ray() {}
    Ray(float3 origin, float3 direction) : orig(origin), dir(direction) {}

    float3 origin() const { return orig; }
    float3 direction() const { return dir; }
    float3 at(float t) const { return orig + t * dir; }
};