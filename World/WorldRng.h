#ifndef WORLD_RNG_H
#define WORLD_RNG_H

#include <stdint.h>

// Small, fast, deterministic RNG (splitmix64). World generation uses this instead of rand() so
// the same seed always gives the same world, and so it is safe to run on a background thread.
struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed = 1) : s(seed * 0x9E3779B97F4A7C15ULL + 0xD1B54A32D192ED03ULL) {}

    uint32_t next() {
        uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return (uint32_t)((z ^ (z >> 31)) >> 32);
    }
    float f() { return (next() >> 8) * (1.0f / 16777216.0f); }          // [0,1)
    int range(int lo, int hi) { return lo + (int)(next() % (uint32_t)(hi - lo + 1)); }  // inclusive
    bool chance(float p) { return f() < p; }
};

#endif
