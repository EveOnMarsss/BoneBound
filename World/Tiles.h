#ifndef TILES_H
#define TILES_H

#include "../project.h"

// ============================================================================
// LOCAL MAP TILE CODES
// Every local-map cell is one char. Natural terrain codes are listed here;
// building / furniture codes (# D B C S A F H P R K Q T) are unchanged.
// NOTE: 'T' is a TABLE (furniture). Trees are lowercase 't' now - the old code
// used 'T' for both, which made tables and trees indistinguishable.
// ============================================================================
#define TILE_GRASS      '.'
#define TILE_TALLGRASS  '"'
#define TILE_FLOWERS    '*'
#define TILE_SAND       ':'
#define TILE_SNOW       'N'
#define TILE_MUD        'm'
#define TILE_SHALLOW    'w'   // wadeable water (rivers fords, shores, marsh pools)
#define TILE_DEEP       '~'   // deep water - blocks movement
#define TILE_TREE       't'
#define TILE_PINE       'Y'
#define TILE_HILL       'h'
#define TILE_ROCK       '^'   // mountain rock - blocks movement
#define TILE_BOULDER    'o'
#define TILE_CACTUS     'c'
#define TILE_BRIDGE     '='
#define TILE_ROAD       ','
#define TILE_WALL       '#'
#define TILE_DOOR       'D'

typedef enum {
    TILEKIND_NONE = 0,
    TILEKIND_GROUND,    // grass-like floor, tinted by biome
    TILEKIND_PLANT,     // trees etc, tinted by biome, sits on grass
    TILEKIND_WATER,
    TILEKIND_ROCK,
    TILEKIND_STRUCT     // walls, furniture, doors
} TileKind;

struct TileDef {
    unsigned char glyph[4];   // CP437 glyph index variants (picked by position hash)
    Color color;              // base color (before biome tint / jitter)
    bool walkable;
    bool water;
    bool buildable;           // buildings may be placed over this (trees get cleared)
    unsigned char kind;
    const char* name;
};

const TileDef& GetTileDef(char tile);
bool  TileWalkable(char tile);          // declared in project.h too
bool  TileIsWater(char tile);
bool  TileBuildable(char tile);
const char* TileName(char tile);

// Rendering helpers. 'biome' is the BiomeType of that exact cell (LocalMap::biome),
// 'h' is a per-cell hash used for glyph variety and color jitter.
int   TileGlyph(char tile, unsigned int h);
Color TileColor(char tile, int biome, unsigned int h);
Color TileBackground(char tile, int biome, unsigned int h);   // alpha 0 = none

Color GetTileColor(char tile);          // legacy convenience (biome-less)

// Stable integer hash for coordinates (used for jitter, scatter, etc.)
unsigned int HashCell(int x, int y, unsigned int seed);

#endif
