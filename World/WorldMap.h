#ifndef WORLD_MAP_H
#define WORLD_MAP_H

#include "../project.h"

// Generation. Begin/Update run the generator on a background thread (loading screen with progress);
// GenerateWorldMap is the blocking version.
void BeginWorldGeneration(int width, int height, unsigned int seed);
bool UpdateWorldGeneration();            // call every frame while generating; true once the world is ready (then pick a start)
void BeginStartSelection(int recommendedX, int recommendedY);   // enter "choose your starting location" mode
void GetRecommendedStart(int* x, int* y);
void GenerateWorldMap(int width, int height);               // blocking, uses worldSeed
void FinalizeNewWorld(int startX, int startY);

extern bool previewWholeMap;
void DrawWorldMap();
void CleanupAllMaps();

// World-map helpers
void WorldTileLook(const WorldTile* t, int* glyph, Color* fg, Color* bg);   // glyph + colours used to draw a world tile
bool WorldTileEnterable(int x, int y);               // can the player walk onto this world tile?
void RevealAround(int cx, int cy, int radius);       // mark tiles as discovered
int  LocationGlyph(LocationType type);
Color LocationColor(LocationType type);

#endif
