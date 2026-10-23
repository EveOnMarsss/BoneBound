#include "SaveLoad.h"
#include "../World/WorldMap.h"
#include "../World/LocalMap.h"
#include "../Game/Camera.h"
#include "../Entities/NPC.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// SAVE FORMAT (version 2)
//   SaveHeader | SaveTile[width*height]
// Local maps are NOT stored: they are generated deterministically from the world seed and the
// world-tile data, so a visited map regenerates exactly as it was. (What the player changes on a
// local map - buildings, items, NPC state - will need a per-tile "delta" record once those exist.)
// ============================================================================
#define SAVE_MAGIC   "BBND"
#define SAVE_VERSION 2u

struct SaveHeader {
    char magic[4];
    unsigned int version;
    unsigned int seed;
    int width, height;

    char name[32];
    unsigned char isCat, isInLocalMap, pad0, pad1;
    int level, currentHealth, maxHealth, gold, xp;
    int npcRelation, factionRelation, playTimeMinutes, questsCompleted;
    int worldX, worldY, localX, localY;
    char locationName[50];
    char pad2[2];
};

struct SaveTile {
    unsigned char biome, riverMask, roadMask, riverWidth;
    unsigned char flags;              // bit0 lake, bit1 discovered, bit2 hasExitPoint
    unsigned char locationType;
    unsigned short riverFlow;
    float elevation, temperature, moisture;
    short exitX, exitY;
    char name[50];
    char pad[2];
};

static void SlotName(int slot, char* out, size_t size) { snprintf(out, size, "save_%d.dat", slot); }

bool SaveFileExists(int slot)
{
    char filename[64]; SlotName(slot, filename, sizeof(filename));
    FILE* f = fopen(filename, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static bool ReadHeader(FILE* f, SaveHeader* h)
{
    if (fread(h, sizeof(SaveHeader), 1, f) != 1) return false;
    if (memcmp(h->magic, SAVE_MAGIC, 4) != 0 || h->version != SAVE_VERSION) return false;
    if (h->width < 2 || h->height < 2 || h->width > 1024 || h->height > 1024) return false;
    h->name[sizeof(h->name) - 1] = '\0';
    h->locationName[sizeof(h->locationName) - 1] = '\0';
    return true;
}

bool ReadSaveInfo(int slot, SaveInfo* info)
{
    char filename[64]; SlotName(slot, filename, sizeof(filename));
    FILE* f = fopen(filename, "rb");
    if (!f) return false;
    SaveHeader h;
    bool ok = ReadHeader(f, &h);
    fclose(f);
    if (!ok) return false;
    memset(info, 0, sizeof(*info));
    snprintf(info->name, sizeof(info->name), "%s", h.name);
    info->isCat = h.isCat != 0;
    info->level = h.level;
    snprintf(info->locationName, sizeof(info->locationName), "%s", h.locationName);
    info->worldWidth = h.width; info->worldHeight = h.height; info->seed = h.seed;
    return true;
}

bool SaveGame(int slot)
{
    if (!worldMap) return false;
    char filename[64]; SlotName(slot, filename, sizeof(filename));
    FILE* f = fopen(filename, "wb");
    if (!f) { LOG("Save failed: cannot open %s\n", filename); return false; }

    SaveHeader h; memset(&h, 0, sizeof(h));
    memcpy(h.magic, SAVE_MAGIC, 4);
    h.version = SAVE_VERSION;
    h.seed = worldSeed;
    h.width = currentMapWidth; h.height = currentMapHeight;
    snprintf(h.name, sizeof(h.name), "%s", player.name);
    h.isCat = player.isCat; h.isInLocalMap = player.isInLocalMap;
    h.level = player.level; h.currentHealth = player.currentHealth; h.maxHealth = player.maxHealth;
    h.gold = player.gold; h.xp = player.xp;
    h.npcRelation = player.npcRelation; h.factionRelation = player.factionRelation;
    h.playTimeMinutes = player.playTimeMinutes; h.questsCompleted = player.questsCompleted;
    h.worldX = player.worldPos.x; h.worldY = player.worldPos.y;
    h.localX = player.localPos.x; h.localY = player.localPos.y;
    snprintf(h.locationName, sizeof(h.locationName), "%s", worldMap[player.worldPos.y][player.worldPos.x].locationName);

    bool ok = fwrite(&h, sizeof(h), 1, f) == 1;
    for (int y = 0; y < currentMapHeight && ok; y++) {
        for (int x = 0; x < currentMapWidth && ok; x++) {
            const WorldTile* t = &worldMap[y][x];
            SaveTile s; memset(&s, 0, sizeof(s));
            s.biome = (unsigned char)t->biome; s.riverMask = t->riverMask; s.roadMask = t->roadMask; s.riverWidth = t->riverWidth;
            s.flags = (unsigned char)((t->isLake ? 1 : 0) | (t->discovered ? 2 : 0) | (t->hasExitPoint ? 4 : 0));
            s.locationType = (unsigned char)t->locationType;
            s.riverFlow = t->riverFlow;
            s.elevation = t->elevation; s.temperature = t->temperature; s.moisture = t->moisture;
            s.exitX = (short)t->lastExitX; s.exitY = (short)t->lastExitY;
            snprintf(s.name, sizeof(s.name), "%s", t->locationName);
            ok = fwrite(&s, sizeof(s), 1, f) == 1;
        }
    }
    fclose(f);
    LOG("Game saved to %s (%s)\n", filename, ok ? "ok" : "WRITE ERROR");
    return ok;
}

bool LoadGame(int slot)
{
    char filename[64]; SlotName(slot, filename, sizeof(filename));
    FILE* f = fopen(filename, "rb");
    if (!f) return false;

    SaveHeader h;
    if (!ReadHeader(f, &h)) {
        LOG("Load failed: %s is damaged or from an incompatible version\n", filename);
        fclose(f);
        return false;
    }

    // Read everything into a temporary world first, so a damaged file can never leave the game half-loaded.
    WorldTile** tmp = (WorldTile**)calloc(h.height, sizeof(WorldTile*));
    bool ok = tmp != NULL;
    for (int y = 0; y < h.height && ok; y++) {
        tmp[y] = (WorldTile*)calloc(h.width, sizeof(WorldTile));      // calloc: no uninitialised fields, ever
        ok = tmp[y] != NULL;
        for (int x = 0; x < h.width && ok; x++) {
            SaveTile s;
            if (fread(&s, sizeof(s), 1, f) != 1 || s.biome >= BIOME_COUNT || s.locationType > LOCATION_TEMPLE) { ok = false; break; }
            WorldTile* t = &tmp[y][x];
            t->biome = (BiomeType)s.biome; t->riverMask = s.riverMask; t->roadMask = s.roadMask; t->riverWidth = s.riverWidth;
            t->isLake = (s.flags & 1) != 0; t->discovered = (s.flags & 2) != 0; t->hasExitPoint = (s.flags & 4) != 0;
            t->locationType = (LocationType)s.locationType;
            t->riverFlow = s.riverFlow;
            t->elevation = s.elevation; t->temperature = s.temperature; t->moisture = s.moisture;
            t->lastExitX = s.exitX; t->lastExitY = s.exitY;
            s.name[sizeof(s.name) - 1] = '\0';
            snprintf(t->locationName, sizeof(t->locationName), "%s", s.name);
            t->hasLocalMap = true;
            switch (t->biome) {
                case BIOME_DEEP_OCEAN: case BIOME_OCEAN: case BIOME_LAKE: t->worldTile = '~'; break;
                case BIOME_FOREST: case BIOME_JUNGLE: case BIOME_TAIGA: t->worldTile = 'T'; break;
                case BIOME_MOUNTAINS: case BIOME_SNOW_PEAKS: t->worldTile = '^'; break;
                default: t->worldTile = '.'; break;
            }
        }
    }
    fclose(f);

    if (!ok) {
        LOG("Load failed: %s is truncated or corrupt\n", filename);
        if (tmp) { for (int y = 0; y < h.height; y++) free(tmp[y]); free(tmp); }
        return false;
    }

    // Commit
    CleanupAllMaps();
    worldMap = tmp;
    currentMapWidth = h.width; currentMapHeight = h.height;
    worldSeed = h.seed;

    memset(&player, 0, sizeof(player));
    snprintf(player.name, sizeof(player.name), "%s", h.name);
    player.isCat = h.isCat != 0;
    player.level = h.level; player.currentHealth = h.currentHealth; player.maxHealth = h.maxHealth;
    player.gold = h.gold; player.xp = h.xp;
    player.npcRelation = h.npcRelation; player.factionRelation = h.factionRelation;
    player.playTimeMinutes = h.playTimeMinutes; player.questsCompleted = h.questsCompleted;
    player.worldPos.x = h.worldX; player.worldPos.y = h.worldY;
    player.localPos.x = h.localX; player.localPos.y = h.localY;
    player.isInLocalMap = false;
    if (player.worldPos.x < 0 || player.worldPos.y < 0 || player.worldPos.x >= h.width || player.worldPos.y >= h.height) {
        player.worldPos.x = h.width / 2; player.worldPos.y = h.height / 2;
    }

    isInConversation = false;
    talkingNPC = NULL;

    if (h.isInLocalMap) {
        // regenerate the local map from the seed and put the player back exactly where they were
        int sx = h.localX, sy = h.localY;
        EnterLocalMap(player.worldPos.x, player.worldPos.y);
        if (player.isInLocalMap) {
            LocalMap* lm = worldMap[player.worldPos.y][player.worldPos.x].localMap;
            if (sx >= 0 && sy >= 0 && sx < lm->width && sy < lm->height && IsTileWalkable(lm->tiles[sy][sx])) {
                player.localPos.x = sx; player.localPos.y = sy;
            }
        }
    }
    SafeInitCamera(true);
    LOG("Game loaded from %s (seed %u, %dx%d)\n", filename, worldSeed, h.width, h.height);
    return true;
}
