#include "SpawnPoint.h"
#include "../World/Tiles.h"
#include "NPC.h"
#include "../World/LocalMap.h"
#include <time.h>

// ===== VILLAGE SPAWN POINTS =====
void CreateTownSpawnPoints(LocalMap* map)
{
    printf("CreateTownSpawnPoints: Starting for village\n");
    
    // Calculate target spawn points
    int targetCount = 5 + (rand() % 5); // 5-10 spawn points
    printf("Target: %d spawn points\n", targetCount);
    
    // Allocate maximum possible first
    map->spawnPoints = (SpawnPoint**)calloc(targetCount, sizeof(SpawnPoint*));
    if (!map->spawnPoints) {
        printf("ERROR: Failed to allocate spawn points array\n");
        map->spawnPointCount = 0;
        return;
    }
    
    int safeMargin = 30;
    int maxX = map->width - safeMargin;
    int maxY = map->height - safeMargin;
    
    int spawnedCount = 0;
    int attempts = 0;
    int maxAttempts = targetCount * 20;
    
    while (spawnedCount < targetCount && attempts < maxAttempts) {
        attempts++;
        
        SpawnPoint* spawn = (SpawnPoint*)calloc(1, sizeof(SpawnPoint));
        if (!spawn) {
            printf("ERROR: Failed to allocate spawn point\n");
            continue;
        }
        
        // Find a clear spot
        int x = safeMargin + (rand() % (maxX - safeMargin));
        int y = safeMargin + (rand() % (maxY - safeMargin));
        
        // Check if this spot is on a walkable tile
        bool foundSpot = false;
        if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
            if (IsTileWalkable(map->tiles[y][x]) && !TileIsWater(map->tiles[y][x])) {   // dry, walkable ground only
                foundSpot = true;
            }
        }
        
        if (!foundSpot) {
            free(spawn);
            continue;
        }
        
        // Check if too close to another spawn point
        bool tooClose = false;
        for (int i = 0; i < spawnedCount; i++) {
            if (map->spawnPoints[i]) {
                int dx = abs(map->spawnPoints[i]->x - x);
                int dy = abs(map->spawnPoints[i]->y - y);
                if (dx < 10 && dy < 10) {
                    tooClose = true;
                    break;
                }
            }
        }
        
        if (tooClose) {
            free(spawn);
            continue;
        }
        
        // Set spawn properties
        spawn->x = x;
        spawn->y = y;
        
        // Randomly assign NPC type for variety
        int typeRoll = rand() % 100;
        if (typeRoll < 60) { // 60% Villagers
            spawn->type = SPAWN_HOUSE;
            spawn->npcType = NPC_VILLAGER;
        } else if (typeRoll < 80) { // 20% Guards
            spawn->type = SPAWN_GUARD_POST;
            spawn->npcType = NPC_GUARD;
        } else if (typeRoll < 90) { // 10% Merchants
            spawn->type = SPAWN_SHOP;
            spawn->npcType = NPC_MERCHANT;
        } else { // 10% Innkeepers
            spawn->type = SPAWN_INN;
            spawn->npcType = NPC_INN_KEEPER;
        }
        
        spawn->isUnique = (rand() % 10 < 2); // 20% unique
        spawn->maxCount = 1 + (rand() % 2); // 1-2 NPCs per spawn
        
        // Generate a REAL name - ALWAYS use the name database, NEVER "Villager"
        const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
        if (spawn->isUnique) {
            const char* last = LAST_NAMES[rand() % LAST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s %s", first, last);
        } else {
            snprintf(spawn->npcName, 50, "%s", first);
        }
        
        spawn->currentCount = 0;
        spawn->respawnTimer = spawn->isUnique ? 0 : (120 + (rand() % 120));
        spawn->respawnCounter = 0;
        
        map->spawnPoints[spawnedCount] = spawn;
        spawnedCount++;
        
        printf("  Spawn point %d: at (%d,%d), name='%s', type=%d, unique=%d, maxCount=%d\n", 
               spawnedCount, x, y, spawn->npcName, spawn->npcType, spawn->isUnique, spawn->maxCount);
    }
    
    map->spawnPointCount = spawnedCount;
    printf("CreateTownSpawnPoints: Complete, created %d spawn points\n", spawnedCount);
}

// ===== CASTLE SPAWN POINTS =====
void CreateCastleSpawnPoints(LocalMap* map)
{
    map->spawnPointCount = 15 + (rand() % 10); // 15-25 spawn points
    map->spawnPoints = (SpawnPoint**)malloc(map->spawnPointCount * sizeof(SpawnPoint*));
    
    int safeMargin = 30;
    int maxX = map->width - safeMargin;
    int maxY = map->height - safeMargin;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = safeMargin + (rand() % (maxX - safeMargin));
        spawn->y = safeMargin + (rand() % (maxY - safeMargin));
        
        int typeRoll = rand() % 100;
        if (typeRoll < 50) { // 50% Guards
            spawn->type = SPAWN_GUARD_POST;
            spawn->npcType = NPC_GUARD;
        } else if (typeRoll < 75) { // 25% Servants/Villagers
            spawn->type = SPAWN_HOUSE;
            spawn->npcType = NPC_VILLAGER;
        } else if (typeRoll < 90) { // 15% Merchants
            spawn->type = SPAWN_SHOP;
            spawn->npcType = NPC_MERCHANT;
        } else { // 10% Blacksmiths
            spawn->type = SPAWN_SHOP;
            spawn->npcType = NPC_BLACKSMITH;
        }
        
        spawn->isUnique = (rand() % 10 < 3); // 30% unique in castle
        spawn->maxCount = 1 + (rand() % 3); // 1-3 NPCs per spawn
        
        // Generate name - ALWAYS use the name database
        const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
        if (spawn->isUnique) {
            const char* last = LAST_NAMES[rand() % LAST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s %s", first, last);
        } else {
            snprintf(spawn->npcName, 50, "%s", first);
        }
        
        spawn->currentCount = 0;
        spawn->respawnTimer = spawn->isUnique ? 0 : (60 + (rand() % 120));
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
        
        printf("  Castle spawn %d: name='%s', type=%d\n", i, spawn->npcName, spawn->npcType);
    }
}

// ===== CAVE SPAWN POINTS =====
void CreateCaveSpawnPoints(LocalMap* map)
{
    map->spawnPointCount = 10 + (rand() % 10); // 10-20 spawn points
    map->spawnPoints = (SpawnPoint**)malloc(map->spawnPointCount * sizeof(SpawnPoint*));
    
    int safeMargin = 20;
    int maxX = map->width - safeMargin;
    int maxY = map->height - safeMargin;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = safeMargin + (rand() % (maxX - safeMargin));
        spawn->y = safeMargin + (rand() % (maxY - safeMargin));
        spawn->type = SPAWN_BANDIT_CAMP;
        
        int creatureRoll = rand() % 100;
        if (creatureRoll < 40) {
            spawn->npcType = NPC_BANDIT;
            snprintf(spawn->npcName, 50, "Goblin");
            spawn->maxCount = 3 + (rand() % 5);
        } else if (creatureRoll < 70) {
            spawn->npcType = NPC_BANDIT;
            snprintf(spawn->npcName, 50, "Kobold");
            spawn->maxCount = 4 + (rand() % 6);
        } else {
            spawn->npcType = NPC_BANDIT;
            snprintf(spawn->npcName, 50, "Bat");
            spawn->maxCount = 5 + (rand() % 8);
        }
        
        spawn->isUnique = (i == 0 && rand() % 3 == 0); // First spawn might be unique boss
        if (spawn->isUnique) {
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            char tempName[50];
            strcpy(tempName, spawn->npcName);
            snprintf(spawn->npcName, 50, "%s the %s", first, tempName);
        }
        
        spawn->currentCount = 0;
        spawn->respawnTimer = 120 + (rand() % 180);
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
}

// ===== WIZARD TOWER SPAWN POINTS =====
void CreateTowerSpawnPoints(LocalMap* map)
{
    map->spawnPointCount = 8 + (rand() % 8); // 8-16 spawn points
    map->spawnPoints = (SpawnPoint**)malloc(map->spawnPointCount * sizeof(SpawnPoint*));
    
    int centerX = map->width / 2;
    int centerY = map->height / 2;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = centerX - 40 + (rand() % 80);
        spawn->y = centerY - 40 + (rand() % 80);
        spawn->type = SPAWN_HOUSE;
        
        if (i == 0) {
            // The wizard (unique)
            spawn->isUnique = true;
            spawn->npcType = NPC_WIZARD;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            const char* last = LAST_NAMES[rand() % LAST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s %s", first, last);
            spawn->maxCount = 1;
            spawn->respawnTimer = 0;
        } else if (i < 4) {
            // Apprentices
            spawn->isUnique = false;
            spawn->npcType = NPC_VILLAGER;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            spawn->maxCount = 2 + (rand() % 3);
            spawn->respawnTimer = 180;
        } else {
            // Magical creatures
            spawn->isUnique = false;
            spawn->npcType = NPC_BANDIT;
            const char* creatures[] = {"Golem", "Familiar", "Elemental", "Imp"};
            snprintf(spawn->npcName, 50, "%s", creatures[rand() % 4]);
            spawn->maxCount = 2 + (rand() % 4);
            spawn->respawnTimer = 240;
        }
        
        spawn->currentCount = 0;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
}

// ===== DUNGEON SPAWN POINTS =====
void CreateDungeonSpawnPoints(LocalMap* map)
{
    map->spawnPointCount = 12 + (rand() % 12); // 12-24 spawn points
    map->spawnPoints = (SpawnPoint**)malloc(map->spawnPointCount * sizeof(SpawnPoint*));
    
    int safeMargin = 20;
    int maxX = map->width - safeMargin;
    int maxY = map->height - safeMargin;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = safeMargin + (rand() % (maxX - safeMargin));
        spawn->y = safeMargin + (rand() % (maxY - safeMargin));
        spawn->type = SPAWN_BANDIT_CAMP;
        
        if (i == 0) { // Boss
            spawn->isUnique = true;
            spawn->npcType = NPC_BANDIT;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s the Dungeon Lord", first);
            spawn->maxCount = 1;
            spawn->respawnTimer = 0;
        } else if (i < 5) { // Guards
            spawn->isUnique = false;
            spawn->npcType = NPC_GUARD;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            spawn->maxCount = 2 + (rand() % 4);
            spawn->respawnTimer = 120;
        } else if (i < 10) { // Undead
            spawn->isUnique = false;
            spawn->npcType = NPC_BANDIT;
            if (rand() % 2 == 0) {
                snprintf(spawn->npcName, 50, "Skeleton");
            } else {
                snprintf(spawn->npcName, 50, "Zombie");
            }
            spawn->maxCount = 3 + (rand() % 5);
            spawn->respawnTimer = 180;
        } else { // Prisoners
            spawn->isUnique = false;
            spawn->npcType = NPC_VILLAGER;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            spawn->maxCount = 2 + (rand() % 3);
            spawn->respawnTimer = 0;
        }
        
        spawn->currentCount = 0;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
}

// ===== RUINS SPAWN POINTS =====
void CreateRuinsSpawnPoints(LocalMap* map)
{
    map->spawnPointCount = 10 + (rand() % 10); // 10-20 spawn points
    map->spawnPoints = (SpawnPoint**)malloc(map->spawnPointCount * sizeof(SpawnPoint*));
    
    int safeMargin = 30;
    int maxX = map->width - safeMargin;
    int maxY = map->height - safeMargin;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = safeMargin + (rand() % (maxX - safeMargin));
        spawn->y = safeMargin + (rand() % (maxY - safeMargin));
        spawn->type = SPAWN_BANDIT_CAMP;
        
        int typeRoll = rand() % 100;
        if (typeRoll < 50) { // 50% Grave Robbers
            spawn->npcType = NPC_BANDIT;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            spawn->maxCount = 2 + (rand() % 5);
            spawn->respawnTimer = 150;
        } else if (typeRoll < 75) { // 25% Explorers
            spawn->npcType = NPC_WANDERER;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            spawn->maxCount = 1 + (rand() % 3);
            spawn->respawnTimer = 200;
        } else { // 25% Hermits
            spawn->npcType = NPC_VILLAGER;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            spawn->maxCount = 1 + (rand() % 2);
            spawn->respawnTimer = 250;
        }
        
        spawn->isUnique = (rand() % 10 < 2); // 20% unique
        if (spawn->isUnique) {
            const char* last = LAST_NAMES[rand() % LAST_NAME_COUNT];
            char tempName[50];
            strcpy(tempName, spawn->npcName);
            snprintf(spawn->npcName, 50, "%s %s", tempName, last);
        }
        
        spawn->currentCount = 0;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
}

// ===== FORT SPAWN POINTS =====
void CreateFortSpawnPoints(LocalMap* map)
{
    map->spawnPointCount = 12 + (rand() % 12); // 12-24 spawn points
    map->spawnPoints = (SpawnPoint**)malloc(map->spawnPointCount * sizeof(SpawnPoint*));
    
    int safeMargin = 30;
    int maxX = map->width - safeMargin;
    int maxY = map->height - safeMargin;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = safeMargin + (rand() % (maxX - safeMargin));
        spawn->y = safeMargin + (rand() % (maxY - safeMargin));
        
        int typeRoll = rand() % 100;
        if (typeRoll < 70) { // 70% Soldiers
            spawn->type = SPAWN_GUARD_POST;
            spawn->npcType = NPC_GUARD;
        } else if (typeRoll < 90) { // 20% Support Staff
            spawn->type = SPAWN_HOUSE;
            spawn->npcType = NPC_VILLAGER;
        } else { // 10% Officers
            spawn->type = SPAWN_HOUSE;
            spawn->npcType = NPC_GUARD;
            spawn->isUnique = true;
        }
        
        const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
        if (spawn->isUnique) {
            const char* last = LAST_NAMES[rand() % LAST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s %s", first, last);
        } else {
            snprintf(spawn->npcName, 50, "%s", first);
        }
        
        spawn->maxCount = 2 + (rand() % 4);
        spawn->currentCount = 0;
        spawn->respawnTimer = spawn->isUnique ? 0 : (90 + (rand() % 120));
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
}

// ===== BANDIT CAMP SPAWN POINTS =====
void CreateBanditCampSpawnPoints(LocalMap* map)
{
    map->spawnPointCount = 3 + (rand() % 4); // 3-7 spawn points
    map->spawnPoints = (SpawnPoint**)malloc(map->spawnPointCount * sizeof(SpawnPoint*));
    
    int safeMargin = 30;
    int maxX = map->width - safeMargin;
    int maxY = map->height - safeMargin;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = safeMargin + (rand() % (maxX - safeMargin));
        spawn->y = safeMargin + (rand() % (maxY - safeMargin));
        spawn->type = SPAWN_BANDIT_CAMP;
        spawn->npcType = NPC_BANDIT;
        
        if (i == 0 && rand() % 2 == 0) {
            spawn->isUnique = true;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s the Bandit Leader", first);
            spawn->maxCount = 1;
            spawn->respawnTimer = 0;
        } else {
            spawn->isUnique = false;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            spawn->maxCount = 2 + (rand() % 4);
            spawn->respawnTimer = 120 + (rand() % 120);
        }
        
        spawn->currentCount = 0;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
}

// ===== TRAVELER SPAWN POINTS =====
void CreateTravelerSpawnPoints(LocalMap* map)
{
    map->spawnPointCount = 2 + (rand() % 4); // 2-6 spawn points
    map->spawnPoints = (SpawnPoint**)malloc(map->spawnPointCount * sizeof(SpawnPoint*));
    
    int safeMargin = 30;
    int maxX = map->width - safeMargin;
    int maxY = map->height - safeMargin;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = safeMargin + (rand() % (maxX - safeMargin));
        spawn->y = safeMargin + (rand() % (maxY - safeMargin));
        spawn->type = SPAWN_TRAVELER;
        
        if (rand() % 3 == 0) {
            spawn->npcType = NPC_BANDIT;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            spawn->maxCount = 1 + (rand() % 2);
        } else {
            spawn->npcType = NPC_WANDERER;
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            spawn->maxCount = 1;
        }
        
        spawn->isUnique = false;
        spawn->currentCount = 0;
        spawn->respawnTimer = 180;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
}

// ===== SPAWN NPCS FROM POINTS =====
void SpawnNPCsFromPoints(LocalMap* map, int worldX, int worldY)
{
    if (!map || !map->spawnPoints) {
        printf("ERROR: map or spawnPoints is NULL in SpawnNPCsFromPoints\n");
        return;
    }
    
    printf("Spawning NPCs for map (%d,%d) with %d spawn points\n", worldX, worldY, map->spawnPointCount);
    
    // First, count how many NPCs already exist in this map
    int existingCount = 0;
    for (int i = 0; i < npcCount; i++) {
        if (npcs[i] && npcs[i]->worldX == worldX && npcs[i]->worldY == worldY) {
            existingCount++;
        }
    }
    
    // If we already have NPCs, don't spawn more
    if (existingCount > 0) {
        printf("Map (%d,%d) already has %d NPCs, skipping spawn\n", worldX, worldY, existingCount);
        return;
    }
    
    // Now spawn NPCs
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = map->spawnPoints[i];
        if (!spawn) {
            printf("ERROR: spawn point %d is NULL\n", i);
            continue;
        }
        
        printf("  Spawn point %d: name='%s', type=%d, maxCount=%d\n", 
               i, spawn->npcName, spawn->npcType, spawn->maxCount);
        
        for (int j = 0; j < spawn->maxCount; j++) {
            // Check global limit
            if (npcCount >= MAX_TOTAL_NPCS) {
                printf("Global NPC limit reached! Cannot spawn more.\n");
                return;
            }
            
            NPC* npc = CreateNPCFromSpawn(spawn, worldX, worldY);
            if (npc) {
                npcs = (NPC**)realloc(npcs, (npcCount + 1) * sizeof(NPC*));
                npcs[npcCount] = npc;
                npcCount++;
                spawn->currentCount++;
                printf("    Spawned NPC: '%s'\n", npc->name);
            } else {
                printf("    Failed to create NPC from spawn\n");
            }
        }
    }
    
    printf("Map (%d,%d) now has %d NPCs\n", worldX, worldY, npcCount - existingCount);
}

// ===== UPDATE SPAWN POINTS =====
void UpdateSpawnPoints()
{
    if (!player.isInLocalMap || !worldMap) return;
    
    LocalMap* map = worldMap[player.worldPos.y][player.worldPos.x].localMap;
    if (!map || !map->spawnPoints) return;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = map->spawnPoints[i];
        if (!spawn) continue;
        
        // Only respawn generic NPCs
        if (!spawn->isUnique && spawn->currentCount < spawn->maxCount && spawn->respawnTimer > 0) {
            spawn->respawnCounter += GetFrameTime();
            
            if (spawn->respawnCounter >= spawn->respawnTimer) {
                int needed = spawn->maxCount - spawn->currentCount;
                for (int j = 0; j < needed; j++) {
                    if (npcCount >= MAX_TOTAL_NPCS) return;
                    
                    NPC* npc = CreateNPCFromSpawn(spawn, player.worldPos.x, player.worldPos.y);
                    if (npc) {
                        npcs = (NPC**)realloc(npcs, (npcCount + 1) * sizeof(NPC*));
                        npcs[npcCount] = npc;
                        npcCount++;
                        spawn->currentCount++;
                    }
                }
                spawn->respawnCounter = 0;
            }
        }
    }
}

// ===== NPC DIED =====
void NPCDied(NPC* npc)
{
    if (!npc || !npc->spawnPoint) return;
    
    npc->spawnPoint->currentCount--;
    
    if (npc->isUnique) {
        // Unique NPCs are gone forever
        npc->spawnPoint->maxCount = 0;
    }
}