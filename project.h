#ifndef PROJECT_H
#define PROJECT_H

// ===== RAYLIB INCLUDES =====
#include <raylib.h>
#include <raymath.h>

// ===== STANDARD LIBRARIES =====
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include "Systems/Logger.h"
// ===== GAME CONSTANTS =====
#define GAME_VERSION "0.0.9"
#define TILE_SIZE 24            // 2x the 12px tileset glyphs (crisp pixels)
#define TILESET_CELL 12
#define LOCAL_MAP_WIDTH 256
#define LOCAL_MAP_HEIGHT 256
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define DEFAULT_WORLD_WIDTH 20
#define DEFAULT_WORLD_HEIGHT 15
#define MAX_NPCS 1000
#define MAX_SPAWN_POINTS 40
#define MAX_SAVE_SLOTS 3
#define ROAD_WIDTH 2
#define MAX_NPCS_PER_MAP 100  // Hard limit per local map
#define MAX_TOTAL_NPCS 5000    // Global limit for all maps

// ===== ENUMS =====
typedef enum {
    SIZE_TINY, SIZE_SMALL, SIZE_MEDIUM,
    SIZE_LARGE, SIZE_HUGE, SIZE_GIGANTIC,
    NUM_SIZES
} MapSize;

typedef enum {
    STATE_TITLE, STATE_MAPSIZE, STATE_SEED,
    STATE_PLAYING, STATE_SAVE_MENU, STATE_LOAD_MENU, STATE_CHARACTER_CREATION,
    STATE_PAUSE, STATE_GENERATING, STATE_CHOOSE_START
} GameState;

typedef enum {
    NPC_VILLAGER, NPC_GUARD, NPC_MERCHANT,
    NPC_INN_KEEPER, NPC_BLACKSMITH, NPC_WANDERER,
    NPC_BANDIT, NPC_WIZARD
} NPCType;

typedef enum {
    NPC_IDLE, NPC_WANDERING, NPC_TALKING,
    NPC_FOLLOWING, NPC_FLEEING, NPC_DEAD
} NPCState;

typedef enum {
    SPAWN_HOUSE, SPAWN_SHOP, SPAWN_INN,
    SPAWN_GUARD_POST, SPAWN_BANDIT_CAMP, SPAWN_TRAVELER
} SpawnType;

// ===== LOCATION TYPES =====
typedef enum {
    LOCATION_NONE = 0,
    LOCATION_VILLAGE,
    LOCATION_CASTLE,
    LOCATION_CAVE,
    LOCATION_WIZARD_TOWER,
    LOCATION_DUNGEON,
    LOCATION_RUINS,
    LOCATION_FORT,
    LOCATION_TEMPLE
    
} LocationType;


// ===== BIOMES (world generation) =====
typedef enum {
    BIOME_DEEP_OCEAN = 0,
    BIOME_OCEAN,
    BIOME_LAKE,
    BIOME_BEACH,
    BIOME_DESERT,
    BIOME_SAVANNA,
    BIOME_GRASSLAND,
    BIOME_FOREST,
    BIOME_JUNGLE,
    BIOME_SWAMP,
    BIOME_TAIGA,
    BIOME_TUNDRA,
    BIOME_GLACIER,
    BIOME_HILLS,
    BIOME_MOUNTAINS,
    BIOME_SNOW_PEAKS,
    BIOME_COUNT
} BiomeType;

#define SEA_LEVEL 0.35f   // elevation (0..1) at which land meets water

// Directions for rivers / roads crossing between world tiles (bit i of a mask = direction i)
// 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW
static const int DIR_DX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static const int DIR_DY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

// ===== FORWARD DECLARATIONS =====
typedef struct WorldTile WorldTile;
typedef struct LocalMap LocalMap;
typedef struct NPC NPC;
typedef struct SpawnPoint SpawnPoint;

// ===== STRUCTURES =====

// Map configuration
typedef struct {
    int width;
    int height;
    const char* name;
    float defaultZoom;
} MapConfig;

// Player position
typedef struct {
    int x, y;
} Position;

// Camera system
typedef struct {
    Vector2 target;
    Vector2 offset;
    float zoom;
    float rotation;
    Camera2D camera;
} GameCamera;

// Spawn Point
struct SpawnPoint {
    int x, y;
    SpawnType type;
    NPCType npcType;
    char npcName[50];
    bool isUnique;
    int respawnTimer;
    int maxCount;
    int currentCount;
    float respawnCounter;
};

// Collision helper - the single source of truth lives in World/Tiles.cpp (tile table)
bool TileWalkable(char tile);
static inline bool IsTileWalkable(char tile) { return TileWalkable(tile); }

// NPC
// Add these enums for conversation
typedef enum {
    DIALOG_GREETING,
    DIALOG_CHAT,
    DIALOG_QUEST,
    DIALOG_TRADE,
    DIALOG_FAREWELL
} DialogState;

// Update NPC struct with conversation fields
struct NPC {
    int id;
    char name[50];
    NPCType type;
    NPCState state;
    
    // Position
    int worldX, worldY;
    int localX, localY;
    bool isInLocalMap;
    
    // Appearance
    char displayChar;
    Color color;
    char greeting[100];
    char chatLines[5][100];      // Multiple chat options
    int chatLineCount;
    char questText[200];          // Quest description if has quest
    char tradeText[100];          // Trade offer text
    char farewell[100];
    
    // AI
    int wanderTimer;
    int wanderDirection;
    float respawnTimer;
    bool isActive;
    
    // Interaction
    bool canTalk;
    bool canTrade;
    bool givesQuests;
    bool hasQuest;                 // Does NPC currently have a quest?
    bool isUnique;
    bool isHostile;
    bool isCat;
    bool isDog;
    
    // Conversation
    bool isTalking;                // Currently talking to player
    DialogState dialogState;
    int selectedOption;
    
    // Stats
    int health;
    int maxHealth;
    int gold;
    int relationship;
    
    // Spawn point reference
    SpawnPoint* spawnPoint;
};

// World Tile
struct WorldTile {
    char worldTile;          // legacy summary glyph ('~' water, 'T' forest, '^' mountain, '.' other)
    LocalMap* localMap;
    bool hasLocalMap;
    LocationType locationType;
    char locationName[50];
    bool discovered;
    int lastExitX;
    int lastExitY;
    bool hasExitPoint;

    // ---- procedural world data (filled by WorldGen) ----
    BiomeType biome;
    float elevation;         // 0..1, SEA_LEVEL = shoreline
    float temperature;       // 0 (polar) .. 1 (tropical)
    float moisture;          // 0 (arid) .. 1 (soaked)
    unsigned char riverMask; // direction bits: a river flows through this tile toward/from those neighbours
    unsigned char roadMask;  // direction bits: a road runs through this tile
    unsigned char riverWidth;// river width in local-map cells (0 = no river)
    unsigned char isLake;
    unsigned short riverFlow;// accumulated flow (for display / debugging)
};

// Player stats
typedef struct {
    Position worldPos;
    Position localPos;
    bool isInLocalMap;
    bool isCat;
    char name[32];
    int currentHealth;
    int maxHealth;
    int gold;
    int xp;
    int level;
    int npcRelation;
    int factionRelation;
    int playTimeMinutes;
    int questsCompleted;
} PlayerData;

// ===== GLOBAL VARIABLES =====
extern WorldTile** worldMap;
extern GameState currentState;
extern PlayerData player;
extern GameCamera gameCamera;
extern NPC** npcs;
extern int npcCount;
extern int currentMapWidth;
extern int currentMapHeight;
extern MapConfig mapSizes[NUM_SIZES];
extern bool shouldQuit;
extern unsigned int worldSeed;
extern int selectedOption;
extern int saveSlotSelected;
extern NPC* talkingNPC;
extern bool isInConversation;
extern int conversationChoice;

// ===== NAME DATABASE DECLARATIONS =====
extern const char* FIRST_NAMES[];
extern const char* LAST_NAMES[];
extern const int FIRST_NAME_COUNT;
extern const int LAST_NAME_COUNT;

#endif
