#ifndef WORLDGEN_H
#define WORLDGEN_H

#include "../project.h"

// ============================================================================
// WORLD GENERATION
// Pipeline: continents -> elevation (+mountain ranges) -> temperature (latitude, altitude)
//           -> rainfall (prevailing winds, ocean moisture, rain shadows) -> hydrology
//           (priority-flood, lakes, flow accumulation, rivers) -> biomes -> settlements
//           -> road network. Everything is derived from one seed.
// ============================================================================

// Shared by the world map and local map generators so both agree on what a biome is.
BiomeType ClassifyBiome(float elevation, float temperature, float moisture);

// Synchronous generation into an already allocated worldMap[height][width].
void WorldGen_Generate(int width, int height, unsigned int seed, WorldTile** map, int* startX, int* startY);

// Threaded generation with progress (used for the loading screen).
void WorldGen_StartAsync(int width, int height, unsigned int seed, WorldTile** map);
bool WorldGen_IsDone();
int  WorldGen_GetProgress();            // 0..100
const char* WorldGen_GetStageName();
void WorldGen_Join();                   // call once IsDone() is true
void WorldGen_GetStart(int* x, int* y); // recommended starting tile

#endif
