#ifndef WORLDGEN_H
#define WORLDGEN_H

#include <string>
#include <vector>
#include "roguelib.h"

using namespace std;

// World size enumeration (mapped to actual dimensions)
typedef enum {
    WORLD_POCKET = 0,    // 16x16 regions
    WORLD_SMALLER,       // 32x32 regions
    WORLD_SMALL,         // 64x64 regions
    WORLD_MEDIUM,        // 128x128 regions
    WORLD_LARGE,         // 256x256 regions
    WORLD_GIGANTIC       // 512x512 regions
} WorldSize;

// History length in years
typedef enum {
    HISTORY_MINIMUM = 50,      // Minimum 50 years (custom)
    HISTORY_SHORT = 100,      // ~100 years
    HISTORY_MEDIUM = 250,     // ~250 years
    HISTORY_LONG = 500,       // ~500 years
    HISTORY_VERY_LONG = 1000, // ~1000 years
    HISTORY_EPIC = 2500       // ~2500 years
} HistoryLength;

// Frequency/amount enumerations (reused across multiple parameters)
typedef enum {
    AMOUNT_NONE = 0,
    AMOUNT_VERY_LOW,
    AMOUNT_LOW,
    AMOUNT_MEDIUM,
    AMOUNT_HIGH,
    AMOUNT_VERY_HIGH
} WorldAmount;

// Terrain parameters struct - matches DF's advanced parameters
typedef struct TerrainParams {
    struct Range {
        int min;
        int max;
        int variance_x;
        int variance_y;
    };
    
    Range elevation;      // 0-400, higher = mountains
    Range rainfall;        // 0-100, higher = wetter
    Range temperature;     // -1000 to 1000, higher = hotter
    Range drainage;        // 0-100, higher = better drained
    Range volcanism;       // 0-100, 100 = volcanoes possible
    Range savagery;        // 0-100, higher = more aggressive beasts
    
    // Weighted ranges (mesh sizes 1-6, weights for 0-20,20-40,40-60,60-80,80-100)
    int elevation_weights[6];  // [mesh, w1, w2, w3, w4, w5]
    int rain_weights[6];
    int drainage_weights[6];
    int temperature_weights[6];
    int savagery_weights[6];
    int volcanism_weights[6];
} TerrainParams;

// Biome type enumeration for mapping to glyphs
typedef enum {
    BIOME_OCEAN = 0,
    BIOME_GLACIER,
    BIOME_TUNDRA,
    BIOME_SWAMP,
    BIOME_MARSH,
    BIOME_DESERT,
    BIOME_BADLAND,
    BIOME_GRASSLAND,
    BIOME_FOREST,
    BIOME_HILLS,
    BIOME_MOUNTAINS,
    BIOME_LAKE,
    BIOME_RIVER,
    BIOME_VOLCANO,
    BIOME_CAVERN,
    BIOME_MAGMA,
    BIOME_EVIL,
    BIOME_GOOD,
    BIOME_SAVAGE,
    BIOME_BEAST_LAIR,
    BIOME_COUNT
} BiomeType;

// Biome glyph mapping - each biome gets 3 glyphs (variant1, variant2, variant3)
typedef struct BiomeGlyphs {
    BiomeType type;
    int glyphs[3];  // Three glyph indices for visual variety
    Color base_color;
    bool is_water;
    bool is_mountain;
} BiomeGlyphs;

// World generation parameters
typedef struct WorldGenParams {
    // Basic parameters
    WorldSize size;
    HistoryLength history_length;
    WorldAmount civilizations;
    WorldAmount max_sites;
    WorldAmount beasts;
    WorldAmount savagery;
    WorldAmount minerals;
    
    // Advanced terrain parameters
    TerrainParams terrain;
    
    // Feature counts
    int min_volcanoes;
    int min_mountain_peaks;
    int min_river_sources;
    int erosion_cycles;
    
    // Cave/cavern parameters
    int cavern_layers;           // 0-3
    int cavern_openness_min;     // 0-100
    int cavern_openness_max;     // 0-100
    int cavern_passage_min;      // 0-100
    int cavern_passage_max;      // 0-100
    int cavern_water_min;        // 0-100
    int cavern_water_max;        // 0-100
    
    // Beast counts
    int megabeast_caves;
    int semimegabeast_caves;
    int titan_number;
    
    // Rejection thresholds (set to 0 to disable)
    struct RejectionParams {
        int min_mountain_tiles;
        int min_forest_tiles;
        int min_desert_tiles;
        int min_swamp_tiles;
        int min_ocean_tiles;
        int max_subregions;
    } reject;
    
    // Seeds for reproducible generation
    unsigned int seed_main;
    unsigned int seed_history;
    unsigned int seed_names;
    unsigned int seed_creatures;
} WorldGenParams;

// Generated world data
typedef struct World {
    char name[64];
    WorldGenParams params;
    
    // World dimensions in tiles (not regions)
    int width_tiles;
    int height_tiles;
    
    // Biome map (2D array of biome types)
    BiomeType* biome_map;
    
    // Elevation map (0-255)
    unsigned char* elevation_map;
    
    // Temperature map (-100 to 100)
    signed char* temperature_map;
    
    // Rainfall map (0-255)
    unsigned char* rainfall_map;
    
    // Drainage map (0-255)
    unsigned char* drainage_map;
    
    // Savagery map (0-255)
    unsigned char* savagery_map;
    
    // Civilization data (simplified)
    int civilization_count;
    void* civilization_data;  // Placeholder for civ system
    
    // Site data
    int site_count;
    void* site_data;  // Placeholder for site system
    
    // Beast data
    int beast_count;
    void* beast_data;  // Placeholder for beast system
} World;

// Initialize world generation with default parameters
WorldGenParams WorldGen_GetDefaultParams(WorldSize size);

// Generate a new world based on parameters
World* WorldGen_Generate(const WorldGenParams* params);

// Free world data
void WorldGen_FreeWorld(World* world);

// Render a world map using roguelib
void WorldGen_RenderWorld(const World* world, Render* render, int screen_x, int screen_y, int width, int height);

// Get biome glyphs for rendering
const BiomeGlyphs* WorldGen_GetBiomeGlyphs(void);

// Convert world coordinates to map index
inline int WorldGen_GetIndex(const World* world, int x, int y) {
    return y * world->width_tiles + x;
}

// Get biome at coordinates
inline BiomeType WorldGen_GetBiome(const World* world, int x, int y) {
    if (x < 0 || x >= world->width_tiles || y < 0 || y >= world->height_tiles)
        return BIOME_OCEAN;
    return world->biome_map[WorldGen_GetIndex(world, x, y)];
}

// Default biome glyph mappings (to be used with roguelib's glyph system)
static const BiomeGlyphs DEFAULT_BIOME_GLYPHS[BIOME_COUNT] = {
    {BIOME_OCEAN,      {247, 247, 126}, BLUE,      true,  false}, // ≈, ≈, ~
    {BIOME_GLACIER,    {176, 177, 178}, SKYBLUE,   false, false}, // ░, ▒, ▓
    {BIOME_TUNDRA,     {176, 177, 250}, LIGHTGRAY, false, false}, // ░, ▒, ·
    {BIOME_SWAMP,      {131, 132, 133}, DARKGREEN, false, false}, // Various swampy chars
    {BIOME_MARSH,      {250, 250, 131}, GREEN,     false, false}, // ·, ·, swamp
    {BIOME_DESERT,     {176, 177, 250}, YELLOW,    false, false}, // ░, ▒, ·
    {BIOME_BADLAND,    {177, 178, 219}, BROWN,     false, false}, // ▒, ▓, █
    {BIOME_GRASSLAND,  {250, 250, 34},  GREEN,     false, false}, // ·, ·, " (quote)
    {BIOME_FOREST,     {167, 168, 169}, DARKGREEN, false, false}, // Various tree chars
    {BIOME_HILLS,      {176, 177, 30},  GREEN,     false, false}, // ░, ▒, ▲
    {BIOME_MOUNTAINS,  {219, 220, 223}, GRAY,      false, true},  // █, ▄, ▀
    {BIOME_LAKE,       {247, 126, 61},  BLUE,      true,  false}, // ≈, ~, =
    {BIOME_RIVER,      {61, 126, 247},  BLUE,      true,  false}, // =, ~, ≈
    {BIOME_VOLCANO,    {94, 94, 219},   RED,       false, true},  // ^, ^, █
    {BIOME_CAVERN,     {176, 177, 219}, DARKGRAY,  false, false}, // ░, ▒, █
    {BIOME_MAGMA,      {219, 220, 94},  ORANGE,    false, false}, // █, ▄, ^
    {BIOME_EVIL,       {131, 133, 165}, PURPLE,    false, false}, // Dark purple tones
    {BIOME_GOOD,       {250, 250, 15},  PINK,      false, false}, // Light/pink tones
    {BIOME_SAVAGE,     {165, 166, 167}, RED,       false, false}, // Reddish tones
    {BIOME_BEAST_LAIR, {165, 166, 219}, DARKGRAY,  false, false}  // Lair markers
};

#endif