#ifndef LOCAL_TERRAIN_H
#define LOCAL_TERRAIN_H

#include "LocalMap.h"

// Fills map->tiles / map->biome for the local map belonging to world tile (wx, wy).
// Everything is a pure function of (worldSeed, world tile data), so a map regenerates identically.
// Adjacent world tiles line up seamlessly (terrain, rivers, roads) because the terrain is sampled from
// continuous fields interpolated between neighbouring world tiles, in global coordinates.
void GenerateLocalTerrain(LocalMap* map, int wx, int wy);

// Spawn/placement helpers for structure code
void ClearTileKeepWater(LocalMap* map, int x, int y);   // grass over obstacles, but never overwrite water/roads
void ClearObstaclesInRect(LocalMap* map, int x0, int y0, int x1, int y1);

#endif
