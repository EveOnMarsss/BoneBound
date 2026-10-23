#ifndef LOCAL_MAP_H
#define LOCAL_MAP_H

#include "../project.h"

// Building types for local maps
typedef enum {
    BUILDING_HOUSE,
    BUILDING_SHOP,
    BUILDING_INN,
    BUILDING_BLACKSMITH,
    BUILDING_GUARD_POST,
    BUILDING_CASTLE_KEEP,
    BUILDING_TOWER,
    BUILDING_TEMPLE,
    BUILDING_DUNGEON_ENTRANCE,
    BUILDING_CAVE_ENTRANCE,
    BUILDING_RUIN,
    BUILDING_FORT_BARRACKS,
    BUILDING_STABLE,
    BUILDING_WAREHOUSE
} BuildingType;

typedef enum 
{
    DOOR_BOTTOM,
    DOOR_TOP,
    DOOR_LEFT,
    DOOR_RIGHT,
    DOOR_RANDOM
} DoorPosition;

// Building structure
typedef struct {
    BuildingType type;
    int x, y;
    int width, height;
    char** layout;
    bool hasDoor;
    int doorX, doorY;
    bool isEnterable;
    char* interiorMap;
} Building;

// Furniture types
typedef enum {
    FURNITURE_TABLE,
    FURNITURE_CHAIR,
    FURNITURE_BED,
    FURNITURE_CHEST,
    FURNITURE_COUNTER,
    FURNITURE_FIREPLACE,
    FURNITURE_SHELF,
    FURNITURE_ANVIL,
    FURNITURE_ALTAR,
    FURNITURE_THRONE,
    FURNITURE_BARREL,
    FURNITURE_CRATE
} FurnitureType;

// Local Map
struct LocalMap {
    char** tiles;
    unsigned char* biome;      // BiomeType per cell (width*height) - used for tinting and ground colour
    int width;
    int height;
    SpawnPoint** spawnPoints;
    int spawnPointCount;
    Building** buildings;
    int buildingCount;
};

// Core functions
void GenerateLocalMap(int worldX, int worldY);
void EnterLocalMap(int worldX, int worldY);
void ExitLocalMap();
void DrawLocalMap();
void RemoveNPCsFromMap(int worldX, int worldY);
bool TravelToAdjacentTile(int dx, int dy);   // seamless travel between world tiles (dx,dy is one of the 4 directions)

// Terrain generation lives in LocalTerrain.cpp (GenerateLocalTerrain)

// Structure placement
void PlaceStructuresForLocation(LocalMap* map, LocationType locationType);
void PlaceVillage(LocalMap* map);
void PlaceCastle(LocalMap* map);
void PlaceCave(LocalMap* map);
void PlaceTower(LocalMap* map);
void PlaceDungeon(LocalMap* map);
void PlaceRuins(LocalMap* map);
void PlaceFort(LocalMap* map);
void PlaceTemple(LocalMap* map);

// Road helper functions
void CreateDirtRoad(LocalMap* map, int x1, int y1, int x2, int y2);
void CreateTownSquareWithRoads(LocalMap* map, int centerX, int centerY, int size);

// Structure definitions
Building* CreateBuilding(BuildingType type, int x, int y, int width, int height);
void AddFurnitureToBuilding(Building* building, FurnitureType furniture, int x, int y);
void PlaceBuildingOnMap(LocalMap* map, Building* building);
Building* CreateHouse(int x, int y);
Building* CreateShop(int x, int y);
Building* CreateInn(int x, int y);
Building* CreateBlacksmith(int x, int y);
Building* CreateGuardPost(int x, int y);
Building* CreateCastleKeep(int x, int y);
Building* CreateTower(int x, int y);
Building* CreateTemple(int x, int y);
Building* CreateFortBarracks(int x, int y);
Building* CreateRuin(int x, int y);
Building* CreateDungeonEntrance(int x, int y);
Building* CreateCaveEntrance(int x, int y);

// Spawn point generation
void GenerateSpawnPoints(LocalMap* map, LocationType locationType);
void GenerateVillageSpawnPoints(LocalMap* map);
void GenerateCastleSpawnPoints(LocalMap* map);
void GenerateCaveSpawnPoints(LocalMap* map);
void GenerateTowerSpawnPoints(LocalMap* map);
void GenerateDungeonSpawnPoints(LocalMap* map);
void GenerateRuinsSpawnPoints(LocalMap* map);
void GenerateFortSpawnPoints(LocalMap* map);
void GenerateBanditCampSpawnPoints(LocalMap* map);
void GenerateTravelerSpawnPoints(LocalMap* map);

// NPC management
void SpawnNPCsForMap(LocalMap* map, int worldX, int worldY);

#endif