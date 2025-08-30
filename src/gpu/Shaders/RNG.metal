#pragma once

struct RNG
{
    uint state;

    void init(uint x) { state = x; }

    uint next_uint()
    {
        state ^= state >> 17;
        state *= 0xed5ad4bb;
        state ^= state >> 11;
        state *= 0xac4c1b51;
        state ^= state >> 15;
        state *= 0x31848bab;
        state ^= state >> 14;
        return state;
    }

    float next_float()
    {
        return float(next_uint()) / 4294967296.0; // [0, 1)
    }
};