#include "WorldMap.h"
#include "LocalMap.h"
#include "WorldGen.h"
#include "Tiles.h"
#include "../Render/Tileset.h"
#include "../Entities/NPC.h"
#include "../Entities/SpawnPoint.h"
#include "../Game/Camera.h"
#include <time.h>

// ============================================================================
// APPEARANCE OF WORLD TILES
// ============================================================================

int LocationGlyph(LocationType type)
{
    switch (type) {
        case LOCATION_VILLAGE:      return 127;   // ⌂
        case LOCATION_CASTLE:       return 206;   // ╬
        case LOCATION_CAVE:         return 239;   // ∩
        case LOCATION_WIZARD_TOWER: return 4;     // ♦
        case LOCATION_DUNGEON:      return 234;   // Ω
        case LOCATION_RUINS:        return 227;   // π
        case LOCATION_FORT:         return 203;   // ╦
        case LOCATION_TEMPLE:       return 15;    // ☼
        default:                    return '?';
    }
}

Color LocationColor(LocationType type)
{
    switch (type) {
        case LOCATION_VILLAGE:      return (Color){ 255, 240, 140, 255 };
        case LOCATION_CASTLE:       return (Color){ 215, 215, 255, 255 };
        case LOCATION_CAVE:         return (Color){ 210, 140,  80, 255 };
        case LOCATION_WIZARD_TOWER: return (Color){ 220, 120, 255, 255 };
        case LOCATION_DUNGEON:      return (Color){ 230,  50,  50, 255 };
        case LOCATION_RUINS:        return (Color){ 190, 190, 190, 255 };
        case LOCATION_FORT:         return (Color){ 160, 160, 170, 255 };
        case LOCATION_TEMPLE:       return (Color){ 255, 215,   0, 255 };
        default:                    return WHITE;
    }
}

static void BiomeLook(BiomeType b, int* glyph, Color* fg)
{
    switch (b) {
        case BIOME_DEEP_OCEAN: *glyph = 247; *fg = (Color){  35,  60, 150, 255 }; break;
        case BIOME_OCEAN:      *glyph = 247; *fg = (Color){  60, 110, 200, 255 }; break;
        case BIOME_LAKE:       *glyph = 247; *fg = (Color){  90, 150, 235, 255 }; break;
        case BIOME_BEACH:      *glyph = 250; *fg = (Color){ 235, 225, 150, 255 }; break;
        case BIOME_DESERT:     *glyph = 176; *fg = (Color){ 225, 190, 105, 255 }; break;
        case BIOME_SAVANNA:    *glyph = 34;  *fg = (Color){ 195, 175,  75, 255 }; break;
        case BIOME_GRASSLAND:  *glyph = 34;  *fg = (Color){ 105, 185,  65, 255 }; break;
        case BIOME_FOREST:     *glyph = 6;   *fg = (Color){  50, 140,  50, 255 }; break;
        case BIOME_JUNGLE:     *glyph = 5;   *fg = (Color){  25, 165,  60, 255 }; break;
        case BIOME_SWAMP:      *glyph = 126; *fg = (Color){ 100, 125,  60, 255 }; break;
        case BIOME_TAIGA:      *glyph = 24;  *fg = (Color){  70, 130, 105, 255 }; break;
        case BIOME_TUNDRA:     *glyph = 249; *fg = (Color){ 160, 180, 185, 255 }; break;
        case BIOME_GLACIER:    *glyph = 177; *fg = (Color){ 225, 240, 255, 255 }; break;
        case BIOME_HILLS:      *glyph = 110; *fg = (Color){ 165, 140,  90, 255 }; break;
        case BIOME_MOUNTAINS:  *glyph = 30;  *fg = (Color){ 150, 140, 130, 255 }; break;
        case BIOME_SNOW_PEAKS: *glyph = 30;  *fg = (Color){ 250, 250, 255, 255 }; break;
        default:               *glyph = '?'; *fg = WHITE; break;
    }
}

// Line glyph for a river through this tile, from its direction mask
static int RiverGlyph(unsigned char m)
{
    auto has = [&](int d) { return (m & (1 << d)) != 0; };
    bool N = has(0), NE = has(1), E = has(2), SE = has(3), S = has(4), SW = has(5), W = has(6), NW = has(7);
    int bits = N + NE + E + SE + S + SW + W + NW;
    if (bits >= 3) return 197;                           // ┼ junction
    if (N && S) return 179;
    if (E && W) return 196;
    if (NE && SW) return 47;
    if (NW && SE) return 92;
    if (N && E) return 192;
    if (N && W) return 217;
    if (S && E) return 218;
    if (S && W) return 191;
    if (N || S) return 179;
    if (E || W) return 196;
    if (NE || SW) return 47;
    if (NW || SE) return 92;
    return 247;
}

static inline Color Scale(Color c, float f)
{
    int r = (int)(c.r * f), g = (int)(c.g * f), b = (int)(c.b * f);
    return (Color){ (unsigned char)(r > 255 ? 255 : r), (unsigned char)(g > 255 ? 255 : g), (unsigned char)(b > 255 ? 255 : b), c.a };
}

void WorldTileLook(const WorldTile* t, int* glyph, Color* fg, Color* bg)
{
    BiomeLook(t->biome, glyph, fg);
    bool water = (t->biome == BIOME_DEEP_OCEAN || t->biome == BIOME_OCEAN || t->biome == BIOME_LAKE);

    if (water) {
        *bg = Scale(*fg, 0.22f);
    } else {
        // gentle relief shading from elevation
        float shade = 0.80f + 0.55f * (t->elevation - SEA_LEVEL);
        if (shade < 0.75f) shade = 0.75f;
        if (shade > 1.15f) shade = 1.15f;
        *fg = Scale(*fg, shade);
        *bg = Scale(*fg, 0.14f);
        if (t->riverMask && !t->isLake) { *glyph = RiverGlyph(t->riverMask); *fg = (Color){ 90, 160, 255, 255 }; }
        else if (t->roadMask)           { *glyph = 250; *fg = (Color){ 205, 155, 85, 255 }; }
    }
    if (t->locationType != LOCATION_NONE) {
        *glyph = LocationGlyph(t->locationType);
        *fg = LocationColor(t->locationType);
    }
    if (!t->discovered && !previewWholeMap) { *fg = Scale(*fg, 0.55f); *bg = Scale(*bg, 0.55f); }
}

bool WorldTileEnterable(int x, int y)
{
    if (!worldMap || x < 0 || y < 0 || x >= currentMapWidth || y >= currentMapHeight) return false;
    BiomeType b = worldMap[y][x].biome;
    return b != BIOME_DEEP_OCEAN && b != BIOME_OCEAN && b != BIOME_LAKE;     // you can't walk across open water
}

void RevealAround(int cx, int cy, int radius)
{
    if (!worldMap) return;
    for (int y = cy - radius; y <= cy + radius; y++)
        for (int x = cx - radius; x <= cx + radius; x++)
            if (x >= 0 && y >= 0 && x < currentMapWidth && y < currentMapHeight &&
                (x - cx) * (x - cx) + (y - cy) * (y - cy) <= radius * radius + 1)
                worldMap[y][x].discovered = true;
}

// ============================================================================
// DRAWING
// ============================================================================

void DrawWorldMap()
{
    if (worldMap == NULL) return;

    if (player.worldPos.y < 0 || player.worldPos.y >= currentMapHeight ||
        player.worldPos.x < 0 || player.worldPos.x >= currentMapWidth) {
        LOG("CRASH PREVENTION: Invalid world position (%d,%d) in DrawWorldMap\n", player.worldPos.x, player.worldPos.y);
        player.worldPos.x = currentMapWidth / 2;
        player.worldPos.y = currentMapHeight / 2;
        return;
    }

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float visibleWidth = screenWidth / gameCamera.camera.zoom;
    float visibleHeight = screenHeight / gameCamera.camera.zoom;

    Vector2 topLeft = {
        gameCamera.camera.target.x - visibleWidth / 2.0f,
        gameCamera.camera.target.y - visibleHeight / 2.0f
    };
    DrawRectangle((int)topLeft.x - 1, (int)topLeft.y - 1, (int)visibleWidth + 2, (int)visibleHeight + 2, BLACK);

    int startX = (int)(topLeft.x / TILE_SIZE) - 1;
    int startY = (int)(topLeft.y / TILE_SIZE) - 1;
    int endX = (int)((topLeft.x + visibleWidth) / TILE_SIZE) + 2;
    int endY = (int)((topLeft.y + visibleHeight) / TILE_SIZE) + 2;
    if (startX < 0) startX = 0;
    if (startY < 0) startY = 0;
    if (endX > currentMapWidth)  endX = currentMapWidth;
    if (endY > currentMapHeight) endY = currentMapHeight;

    // zoomed far out the glyphs turn to mush: draw plain colour blocks instead (a proper "atlas" view)
    const bool coarse = (TILE_SIZE * gameCamera.camera.zoom) < 9.0f;

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            const WorldTile* t = &worldMap[y][x];
            int glyph; Color fg, bg;
            WorldTileLook(t, &glyph, &fg, &bg);
            if (coarse) {
                Color fill = bg.a ? Scale(fg, 0.75f) : fg;
                if (t->biome == BIOME_DEEP_OCEAN || t->biome == BIOME_OCEAN || t->biome == BIOME_LAKE) fill = Scale(fg, 0.8f);
                if (t->riverMask && !t->isLake && t->biome > BIOME_LAKE) fill = (Color){ 80, 150, 255, 255 };
                if (t->locationType != LOCATION_NONE) fill = LocationColor(t->locationType);
                if (!t->discovered && !previewWholeMap) fill = Scale(fill, 0.6f);
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, fill);
            } else {
                DrawGlyphCell(glyph, x, y, fg, bg);
            }
        }
    }
}

// ============================================================================
// GENERATION
// ============================================================================

static void AllocateWorld(int width, int height)
{
    CleanupAllMaps();
    currentMapWidth = width;
    currentMapHeight = height;
    worldMap = (WorldTile**)calloc(height, sizeof(WorldTile*));
    for (int y = 0; y < height; y++) worldMap[y] = (WorldTile*)calloc(width, sizeof(WorldTile));   // calloc: no uninitialised fields
}

void FinalizeNewWorld(int startX, int startY)
{
    previewWholeMap = false;
    player.worldPos.x = startX;
    player.worldPos.y = startY;
    player.isInLocalMap = false;
    player.localPos.x = LOCAL_MAP_WIDTH / 2;
    player.localPos.y = LOCAL_MAP_HEIGHT / 2;
    if (player.maxHealth <= 0) {        // brand new character: starting stats
        player.maxHealth = 20;
        player.currentHealth = 20;
        player.level = 1;
        player.xp = 0;
        player.gold = 5;
    }
    RevealAround(startX, startY, 2);
    LOG("World ready: %dx%d seed %u, start (%d,%d) '%s'\n", currentMapWidth, currentMapHeight, worldSeed,
        startX, startY, worldMap[startY][startX].locationName);
    SafeInitCamera(true);
}

void BeginWorldGeneration(int width, int height, unsigned int seed)
{
    worldSeed = seed;
    AllocateWorld(width, height);
    WorldGen_StartAsync(width, height, seed, worldMap);
}

static int g_recX = 0, g_recY = 0;

void GetRecommendedStart(int* x, int* y) { if (x) *x = g_recX; if (y) *y = g_recY; }

// The generator suggests a good start (a friendly village near water); the player may pick any land tile instead.
void BeginStartSelection(int recommendedX, int recommendedY)
{
    g_recX = recommendedX; g_recY = recommendedY;
    previewWholeMap = true;                 // let the player see the entire world while choosing
    player.worldPos.x = recommendedX;       // the cursor IS player.worldPos, so camera/draw code just works
    player.worldPos.y = recommendedY;
    player.isInLocalMap = false;
    SafeInitCamera(true);
    gameCamera.zoom = FitWorldZoom();       // start zoomed out to the whole map
    gameCamera.camera.zoom = gameCamera.zoom;
}

bool UpdateWorldGeneration()
{
    if (!WorldGen_IsDone()) return false;
    WorldGen_Join();
    int sx, sy;
    WorldGen_GetStart(&sx, &sy);
    BeginStartSelection(sx, sy);
    return true;
}

void GenerateWorldMap(int width, int height)
{
    AllocateWorld(width, height);
    int sx = 0, sy = 0;
    WorldGen_Generate(width, height, worldSeed, worldMap, &sx, &sy);
    FinalizeNewWorld(sx, sy);
}

void CleanupAllMaps()
{
    WorldGen_Join();       // never free the world while the generator thread is still writing to it
    if (!worldMap) return;

    for (int y = 0; y < currentMapHeight; y++) {
        for (int x = 0; x < currentMapWidth; x++) {
            LocalMap* local = worldMap[y][x].localMap;
            if (!local) continue;

            if (local->spawnPoints) {
                for (int i = 0; i < local->spawnPointCount; i++) free(local->spawnPoints[i]);
                free(local->spawnPoints);
            }
            if (local->buildings) {
                for (int i = 0; i < local->buildingCount; i++) {
                    Building* b = local->buildings[i];
                    if (!b) continue;
                    for (int j = 0; j < b->height; j++) free(b->layout[j]);
                    free(b->layout);
                    free(b);
                }
                free(local->buildings);
            }
            for (int ly = 0; ly < local->height; ly++) free(local->tiles[ly]);
            free(local->tiles);
            free(local->biome);
            free(local);
        }
        free(worldMap[y]);
    }
    free(worldMap);
    worldMap = NULL;

    for (int i = 0; i < npcCount; i++) free(npcs[i]);
    free(npcs);
    npcs = NULL;
    npcCount = 0;
}
