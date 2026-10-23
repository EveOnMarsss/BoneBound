#include "LocalMap.h"
#include "LocalTerrain.h"
#include "Tiles.h"
#include "WorldMap.h"
#include "../Render/Tileset.h"
#include "../Entities/SpawnPoint.h"
#include "../Entities/NPC.h"
#include "../Game/Camera.h"
#include "../Systems/Logger.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

// ============================================================================
// CONSTANTS
// ============================================================================

#define VILLAGE_CENTER_X (LOCAL_MAP_WIDTH / 2)
#define VILLAGE_CENTER_Y (LOCAL_MAP_HEIGHT / 2)


// ============================================================================
// BUILDING SIZE RANGES
// ============================================================================

typedef struct {
    int minWidth;
    int maxWidth;
    int minHeight;
    int maxHeight;
} BuildingSizeRange;

const BuildingSizeRange HOUSE_SIZES[] = {
    {8, 10, 5, 6},    // Small house (cottage)
    {8, 10, 5, 6},   // Medium house
    {10, 12, 6, 7},  // Large house
    {12, 14, 7, 8}   // Manor house
};

const BuildingSizeRange SHOP_SIZES[] = {
    {8, 10, 5, 6},   // Small shop
    {10, 12, 6, 7},  // Medium shop
    {12, 14, 6, 7}   // Large shop
};

const BuildingSizeRange INN_SIZES[] = {
    {12, 14, 7, 8},  // Small inn
    {14, 16, 8, 9},  // Medium inn
    {16, 18, 9, 10}  // Large inn
};

const BuildingSizeRange BLACKSMITH_SIZES[] = {
    {10, 12, 6, 7},  // Small smithy
    {12, 14, 7, 8},  // Medium smithy
    {14, 16, 8, 9}   // Large smithy
};

const BuildingSizeRange GUARD_POST_SIZES[] = {
    {6, 8, 4, 5},    // Small outpost
    {8, 10, 5, 6},   // Medium guard post
    {10, 12, 6, 7}   // Large barracks
};

const BuildingSizeRange TOWER_SIZES[] = {
    {8, 10, 8, 10},  // Small tower
    {10, 12, 10, 12}, // Medium tower
    {12, 14, 12, 14}  // Large tower
};

const BuildingSizeRange TEMPLE_SIZES[] = {
    {12, 14, 8, 10},  // Small temple
    {14, 16, 10, 12}, // Medium temple
    {16, 18, 12, 14}  // Large temple
};

const BuildingSizeRange CASTLE_KEEP_SIZES[] = {
    {18, 22, 14, 18}, // Small keep
    {22, 26, 18, 22}, // Medium keep
    {26, 30, 22, 26}  // Large keep
};

const BuildingSizeRange DUNGEON_ENTRANCE_SIZES[] = {
    {8, 10, 5, 6},    // Small dungeon
    {10, 12, 6, 7},   // Medium dungeon
    {12, 14, 7, 8}    // Large dungeon
};

const BuildingSizeRange CAVE_ENTRANCE_SIZES[] = {
    {10, 14, 6, 8},   // Small cave
    {14, 18, 8, 10},  // Medium cave
    {18, 22, 10, 12}  // Large cave
};

const BuildingSizeRange RUIN_SIZES[] = {
    {8, 12, 6, 8},    // Small ruin
    {12, 16, 8, 10},  // Medium ruin
    {16, 20, 10, 12}  // Large ruin
};

// ============================================================================
// COLOR FUNCTIONS
// ============================================================================


// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool IsAreaFree(LocalMap* map, int x, int y, int width, int height)
{
    if (x < 10 || x + width > map->width - 10 || 
        y < 10 || y + height > map->height - 10) {
        return false;
    }
    
    for (int by = -2; by < height + 2; by++) {
        for (int bx = -2; bx < width + 2; bx++) {
            int checkX = x + bx;
            int checkY = y + by;
            if (checkX >= 0 && checkX < map->width && checkY >= 0 && checkY < map->height) {
                char tile = map->tiles[checkY][checkX];
                if (!TileBuildable(tile)) {      // water, rock faces and existing structures block building
                    return false;
                }
            }
        }
    }
    return true;
}

DoorPosition GetDoorPositionForBuilding(int buildingX, int buildingY, int centerX, int centerY)
{
    int dx = buildingX - centerX;
    int dy = buildingY - centerY;
    
    // If building is exactly at center (should never happen, but just in case)
    if (dx == 0 && dy == 0) {
        return DOOR_BOTTOM; // Default
    }
    
    if (abs(dy) > abs(dx) * 1.5) {
        if (dy > 0) return DOOR_TOP;
        else return DOOR_BOTTOM;
    } else if (abs(dx) > abs(dy) * 1.5) {
        if (dx > 0) return DOOR_LEFT;
        else return DOOR_RIGHT;
    } else {
        int roll = rand() % 100;
        if (roll < 40) {
            if (abs(dy) > abs(dx)) {
                return (dy > 0) ? DOOR_TOP : DOOR_BOTTOM;
            } else {
                return (dx > 0) ? DOOR_LEFT : DOOR_RIGHT;
            }
        } else {
            return DOOR_RANDOM;
        }
    }
}
// ============================================================================
// CORE BUILDING FUNCTIONS
// ============================================================================

Building* CreateBuilding(BuildingType type, int x, int y, int width, int height)
{
    LOG("    CreateBuilding: type=%d, x=%d, y=%d, width=%d, height=%d\n", 
        type, x, y, width, height);
    
    // Validate inputs
    if (width <= 0 || height <= 0) {
        LOG("    ERROR: Invalid dimensions width=%d, height=%d\n", width, height);
        return NULL;
    }
    
    if (width > 50 || height > 50) {
        LOG("    ERROR: Dimensions too large width=%d, height=%d\n", width, height);
        return NULL;
    }
    
    Building* building = (Building*)malloc(sizeof(Building));
    if (!building) {
        LOG("    ERROR: Failed to allocate Building structure\n");
        return NULL;
    }
    
    memset(building, 0, sizeof(Building));
    
    building->type = type;
    building->x = x;
    building->y = y;
    building->width = width;
    building->height = height;
    building->hasDoor = false;
    building->doorX = x + width/2;
    building->doorY = y + height - 1;
    building->isEnterable = true;
    building->interiorMap = NULL;
    
    LOG("    Allocating layout with %d rows\n", height);
    building->layout = (char**)malloc(height * sizeof(char*));
    if (!building->layout) {
        LOG("    ERROR: Failed to allocate layout rows\n");
        free(building);
        return NULL;
    }
    
    for (int i = 0; i < height; i++) {
        building->layout[i] = (char*)malloc((width + 1) * sizeof(char));
        if (!building->layout[i]) {
            LOG("    ERROR: Failed to allocate layout row %d\n", i);
            for (int j = 0; j < i; j++) free(building->layout[j]);
            free(building->layout);
            free(building);
            return NULL;
        }
        memset(building->layout[i], '.', width);
        building->layout[i][width] = '\0';
    }
    
    // Set walls
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (i == 0 || i == height-1 || j == 0 || j == width-1) {
                building->layout[i][j] = '#';
            }
        }
    }
    
    LOG("    Building created successfully\n");
    return building;
}

void AddFurnitureToBuilding(Building* building, FurnitureType furniture, int x, int y)
{
    if (!building) {
        LOG("      ERROR: AddFurnitureToBuilding - building is NULL\n");
        return;
    }
    
    if (!building->layout) {
        LOG("      ERROR: AddFurnitureToBuilding - building->layout is NULL\n");
        return;
    }
    
    // Validate coordinates
    if (x < 0 || x >= building->width || y < 0 || y >= building->height) {
        LOG("      ERROR: AddFurnitureToBuilding - coordinates (%d,%d) out of bounds for building %dx%d\n", 
            x, y, building->width, building->height);
        return;
    }
    
    // Check if spot is empty
    if (building->layout[y][x] != '.') {
        LOG("      Warning: Spot (%d,%d) already contains '%c'\n", x, y, building->layout[y][x]);
        return;
    }
    
    // Place furniture
    char furnChar = '?';
    switch (furniture) {
        case FURNITURE_TABLE:    furnChar = 'T'; break;
        case FURNITURE_CHAIR:    furnChar = 'C'; break;
        case FURNITURE_BED:      furnChar = 'B'; break;
        case FURNITURE_CHEST:    furnChar = 'H'; break;
        case FURNITURE_COUNTER:  furnChar = 'S'; break;
        case FURNITURE_FIREPLACE: furnChar = 'F'; break;
        case FURNITURE_SHELF:    furnChar = 'S'; break;
        case FURNITURE_ANVIL:    furnChar = 'A'; break;
        case FURNITURE_ALTAR:    furnChar = 'R'; break;
        case FURNITURE_THRONE:   furnChar = 'K'; break;
        case FURNITURE_BARREL:   furnChar = 'Q'; break;
        case FURNITURE_CRATE:    furnChar = 'Q'; break;
    }
    
    building->layout[y][x] = furnChar;
    LOG("      Placed '%c' at (%d,%d)\n", furnChar, x, y);
}

void PlaceBuildingOnMap(LocalMap* map, Building* building)
{
    LOG("    PlaceBuildingOnMap: building at (%d,%d) size %dx%d\n", 
        building->x, building->y, building->width, building->height);
    
    if (!map) {
        LOG("    ERROR: map is NULL in PlaceBuildingOnMap\n");
        return;
    }
    
    if (!building) {
        LOG("    ERROR: building is NULL in PlaceBuildingOnMap\n");
        return;
    }
    
    if (!map->tiles) {
        LOG("    ERROR: map->tiles is NULL in PlaceBuildingOnMap\n");
        return;
    }
    
    if (!building->layout) {
        LOG("    ERROR: building->layout is NULL in PlaceBuildingOnMap\n");
        return;
    }
    
    int tilesPlaced = 0;
    
    for (int by = 0; by < building->height; by++) {
        for (int bx = 0; bx < building->width; bx++) {
            int mapX = building->x + bx;
            int mapY = building->y + by;
            
            // Validate map coordinates
            if (mapX >= 0 && mapX < map->width && mapY >= 0 && mapY < map->height) {
                // Validate building layout coordinates
                if (by < building->height && bx < building->width) {
                    char tile = building->layout[by][bx];
                    map->tiles[mapY][mapX] = tile;
                    tilesPlaced++;
                    
                    // Log first few tiles for debugging
                    if (tilesPlaced <= 5) {
                        LOG("      Placed tile '%c' at map (%d,%d) from building layout[%d][%d]\n", 
                            tile, mapX, mapY, by, bx);
                    }
                } else {
                    LOG("      ERROR: Building layout index out of bounds: [%d][%d] for size %dx%d\n", 
                        by, bx, building->height, building->width);
                }
            } else {
                LOG("      WARNING: Map coordinates out of bounds: (%d,%d) for map %dx%d\n", 
                    mapX, mapY, map->width, map->height);
            }
        }
    }
    
    LOG("    PlaceBuildingOnMap: placed %d tiles\n", tilesPlaced);
}

void SetBuildingDoor(Building* building, DoorPosition doorPos)
{
    LOG("    SetBuildingDoor: building at (%d,%d), doorPos=%d\n", 
        building->x, building->y, doorPos);
    
    if (!building) {
        LOG("    ERROR: building is NULL in SetBuildingDoor\n");
        return;
    }
    
    if (!building->layout) {
        LOG("    ERROR: building->layout is NULL in SetBuildingDoor\n");
        return;
    }
    
    LOG("    Building size: %dx%d\n", building->width, building->height);
    
    // Clear any existing doors
    int doorsCleared = 0;
    for (int y = 0; y < building->height; y++) {
        for (int x = 0; x < building->width; x++) {
            if (building->layout[y][x] == 'D') {
                building->layout[y][x] = '.';
                doorsCleared++;
            }
        }
    }
    if (doorsCleared > 0) {
        LOG("    Cleared %d existing doors\n", doorsCleared);
    }
    
    int doorX = building->x;
    int doorY = building->y;
    int layoutX = 0, layoutY = 0;
    
    switch (doorPos) {
        case DOOR_BOTTOM:
            layoutX = building->width / 2;
            layoutY = building->height - 1;
            doorX = building->x + layoutX;
            doorY = building->y + layoutY;
            LOG("    Door at BOTTOM: layout[%d][%d]\n", layoutY, layoutX);
            break;
            
        case DOOR_TOP:
            layoutX = building->width / 2;
            layoutY = 0;
            doorX = building->x + layoutX;
            doorY = building->y;
            LOG("    Door at TOP: layout[%d][%d]\n", layoutY, layoutX);
            break;
            
        case DOOR_LEFT:
            layoutX = 0;
            layoutY = building->height / 2;
            doorX = building->x;
            doorY = building->y + layoutY;
            LOG("    Door at LEFT: layout[%d][%d]\n", layoutY, layoutX);
            break;
            
        case DOOR_RIGHT:
            layoutX = building->width - 1;
            layoutY = building->height / 2;
            doorX = building->x + layoutX;
            doorY = building->y + layoutY;
            LOG("    Door at RIGHT: layout[%d][%d]\n", layoutY, layoutX);
            break;
            
        case DOOR_RANDOM:
            {
                int side = rand() % 4;
                LOG("    Random door roll: %d\n", side);
                switch (side) {
                    case 0: // BOTTOM
                        layoutX = building->width / 2;
                        layoutY = building->height - 1;
                        doorX = building->x + layoutX;
                        doorY = building->y + layoutY;
                        LOG("    Random door at BOTTOM: layout[%d][%d]\n", layoutY, layoutX);
                        break;
                    case 1: // TOP
                        layoutX = building->width / 2;
                        layoutY = 0;
                        doorX = building->x + layoutX;
                        doorY = building->y;
                        LOG("    Random door at TOP: layout[%d][%d]\n", layoutY, layoutX);
                        break;
                    case 2: // LEFT
                        layoutX = 0;
                        layoutY = building->height / 2;
                        doorX = building->x;
                        doorY = building->y + layoutY;
                        LOG("    Random door at LEFT: layout[%d][%d]\n", layoutY, layoutX);
                        break;
                    case 3: // RIGHT
                        layoutX = building->width - 1;
                        layoutY = building->height / 2;
                        doorX = building->x + layoutX;
                        doorY = building->y + layoutY;
                        LOG("    Random door at RIGHT: layout[%d][%d]\n", layoutY, layoutX);
                        break;
                }
            }
            break;
    }
    
    // Validate layout coordinates
    if (layoutY >= 0 && layoutY < building->height && 
        layoutX >= 0 && layoutX < building->width) {
        
        // Check if the spot is a wall (should be '#')
        if (building->layout[layoutY][layoutX] == '#') {
            building->layout[layoutY][layoutX] = 'D';
            LOG("    Door placed successfully at layout[%d][%d]\n", layoutY, layoutX);
        } else {
            LOG("    WARNING: Door location at layout[%d][%d] is '%c', not a wall. Overwriting.\n", 
                layoutY, layoutX, building->layout[layoutY][layoutX]);
            building->layout[layoutY][layoutX] = 'D';
        }
    } else {
        LOG("    ERROR: Door layout coordinates out of bounds: [%d][%d] for building %dx%d\n", 
            layoutY, layoutX, building->height, building->width);
        return;
    }
    
    building->doorX = doorX;
    building->doorY = doorY;
    building->hasDoor = true;
    
    LOG("    SetBuildingDoor: door at world (%d,%d), layout (%d,%d)\n", 
        doorX, doorY, layoutX, layoutY);
}

// ============================================================================
// BUILDING CREATION FUNCTIONS
// ============================================================================

Building* CreateHouse(int x, int y, int wealthLevel)
{
    LOG("    CreateHouse: x=%d, y=%d, wealthLevel=%d\n", x, y, wealthLevel);
    
    // Clamp wealth level
    wealthLevel = (wealthLevel < 0) ? 0 : (wealthLevel > 3) ? 3 : wealthLevel;
    
    // Get size range
    int minW = HOUSE_SIZES[wealthLevel].minWidth;
    int maxW = HOUSE_SIZES[wealthLevel].maxWidth;
    int minH = HOUSE_SIZES[wealthLevel].minHeight;
    int maxH = HOUSE_SIZES[wealthLevel].maxHeight;
    
    int width = minW + (rand() % (maxW - minW + 1));
    int height = minH + (rand() % (maxH - minH + 1));
    
    Building* house = CreateBuilding(BUILDING_HOUSE, x, y, width, height);
    if (!house) {
        LOG("    ERROR: CreateBuilding failed\n");
        return NULL;
    }
    
    LOG("    Adding furniture to house...\n");
    
    // Clear the interior
    for (int i = 1; i < height-1; i++) {
        for (int j = 1; j < width-1; j++) {
            house->layout[i][j] = '.';
        }
    }
    
    // ===== SMALL HOUSE LAYOUT (wealthLevel 0-1) =====
    if (wealthLevel <= 1) {
        // Corner 1: Bed in top-left corner
        if (height > 3 && width > 3) {
            house->layout[2][2] = 'B'; // Bed in corner
        }
        
        // Corner 2: Table in top-right corner
        if (height > 3 && width > 4) {
            house->layout[2][width-3] = 'T'; // Table
            // Chair next to table
            if (width > 5) {
                house->layout[2][width-4] = 'C';
            }
        }
        
        // Corner 3: Chest in bottom-left corner
        if (height > 4 && width > 3) {
            house->layout[height-3][2] = 'H'; // Chest
        }
        
        // Corner 4: Fireplace or barrel in bottom-right corner
        if (height > 4 && width > 4) {
            if (wealthLevel == 1) {
                house->layout[height-3][width-3] = 'F'; // Fireplace for wealthier
            } else {
                house->layout[height-3][width-3] = 'Q'; // Barrel for poor
            }
        }
    }
    
    // ===== MEDIUM HOUSE LAYOUT (wealthLevel 2) =====
    else if (wealthLevel == 2) {
        // Bedroom area (top)
        house->layout[2][2] = 'B'; // Bed 1
        if (width > 6) {
            house->layout[2][width-3] = 'B'; // Bed 2
        }
        
        // Chests near beds
        house->layout[3][2] = 'H'; // Chest near bed 1
        if (width > 6) {
            house->layout[3][width-3] = 'H'; // Chest near bed 2
        }
        
        // Living area (bottom)
        int tableX = width / 2;
        int tableY = height - 3;
        house->layout[tableY][tableX] = 'T'; // Table
        
        // Chairs around table
        if (tableX-1 >= 2) house->layout[tableY][tableX-1] = 'C';
        if (tableX+1 < width-2) house->layout[tableY][tableX+1] = 'C';
        if (tableY-1 >= 3) house->layout[tableY-1][tableX] = 'C';
        
        // Fireplace in corner
        house->layout[height-3][2] = 'F';
    }
    
    // ===== LARGE HOUSE LAYOUT (wealthLevel 3) =====
    else {
        // Bedroom area (top) with multiple beds
        int bedSpacing = width / 4;
        for (int i = 0; i < 3; i++) {
            int bedX = bedSpacing * (i + 1);
            if (bedX >= 2 && bedX < width-2) {
                house->layout[2][bedX] = 'B';
                // Chest next to each bed
                if (bedX+1 < width-2) {
                    house->layout[3][bedX] = 'H';
                }
            }
        }
        
        // Living area with table and multiple chairs
        int tableX = width / 2;
        int tableY = height - 4;
        house->layout[tableY][tableX] = 'T';
        
        // Multiple chairs around table
        house->layout[tableY][tableX-1] = 'C';
        house->layout[tableY][tableX+1] = 'C';
        house->layout[tableY-1][tableX] = 'C';
        house->layout[tableY-1][tableX-1] = 'C';
        house->layout[tableY-1][tableX+1] = 'C';
        
        // Kitchen area
        int kitchenX = width - 3;
        int kitchenY = height / 2;
        house->layout[kitchenY][kitchenX] = 'F'; // Fireplace
        house->layout[kitchenY][kitchenX-1] = 'S'; // Shelf
        house->layout[kitchenY+1][kitchenX] = 'Q'; // Barrel
        
        // Extra storage
        house->layout[height-3][2] = 'Q'; // Barrel in corner
        house->layout[2][width-2] = 'H'; // Chest in corner
    }
    
    LOG("    House created successfully\n");
    return house;
}

Building* CreateShop(int x, int y, int shopLevel)
{
    LOG("    CreateShop: x=%d, y=%d, shopLevel=%d\n", x, y, shopLevel);
    
    shopLevel = (shopLevel < 0) ? 0 : (shopLevel > 2) ? 2 : shopLevel;
    
    int minW = SHOP_SIZES[shopLevel].minWidth;
    int maxW = SHOP_SIZES[shopLevel].maxWidth;
    int minH = SHOP_SIZES[shopLevel].minHeight;
    int maxH = SHOP_SIZES[shopLevel].maxHeight;
    
    int width = minW + (rand() % (maxW - minW + 1));
    int height = minH + (rand() % (maxH - minH + 1));
    
    Building* shop = CreateBuilding(BUILDING_SHOP, x, y, width, height);
    if (!shop) {
        LOG("    ERROR: CreateBuilding failed for shop\n");
        return NULL;
    }
    
    LOG("    Adding shop features...\n");
    
    // Clear interior
    for (int i = 1; i < height-1; i++) {
        for (int j = 1; j < width-1; j++) {
            shop->layout[i][j] = '.';
        }
    }
    
    // ===== COUNTER =====
    // Place counter along one wall (right side)
    int counterX = width - 3;
    for (int yPos = 2; yPos < height-2; yPos++) {
        shop->layout[yPos][counterX] = 'S'; // Shelf/counter
    }
    
    // ===== MERCHANT'S CHAIR =====
    // Chair behind counter
    int chairY = height / 2;
    shop->layout[chairY][counterX-1] = 'C';
    
    // ===== TABLE =====
    // Table in the middle or left side
    int tableX = 3;
    int tableY = height / 2;
    shop->layout[tableY][tableX] = 'T';
    
    // ===== SHELVES ALONG WALLS =====
    // Shelves on left wall
    for (int yPos = 2; yPos < height-2; yPos += 2) {
        shop->layout[yPos][2] = 'S';
    }
    
    // Shelves on back wall
    for (int xPos = 2; xPos < width-4; xPos += 2) {
        shop->layout[2][xPos] = 'S';
    }
    
    // ===== GOODS ON SHELVES =====
    int numGoods = 3 + shopLevel * 2;
    
    // Place goods on shelves
    for (int g = 0; g < numGoods; g++) {
        int attempts = 0;
        bool placed = false;
        
        while (!placed && attempts < 20) {
            attempts++;
            
            int shelfX, shelfY;
            
            // Randomly choose a shelf location
            if (rand() % 3 == 0) {
                // Left wall shelf
                shelfX = 2;
                shelfY = 2 + (rand() % (height - 4));
            } else if (rand() % 2 == 0) {
                // Back wall shelf
                shelfX = 2 + (rand() % (width - 5));
                shelfY = 2;
            } else {
                // Counter shelf
                shelfX = counterX;
                shelfY = 2 + (rand() % (height - 4));
            }
            
            // Check bounds and if it's a shelf
            if (shelfX >= 0 && shelfX < width && shelfY >= 0 && shelfY < height) {
                if (shop->layout[shelfY][shelfX] == 'S') {
                    shop->layout[shelfY][shelfX] = 'Q'; // Turn shelf into goods
                    placed = true;
                }
            }
        }
    }
    
    // ===== STORAGE IN BACK =====
    if (height > 5) {
        int storageY = height - 3;
        for (int s = 0; s < 2 + shopLevel; s++) {
            int storageX = 3 + (rand() % (width - 5));
            if (shop->layout[storageY][storageX] == '.') {
                shop->layout[storageY][storageX] = 'Q';
            }
        }
    }
    
    LOG("    Shop created successfully\n");
    return shop;
}

Building* CreateInn(int x, int y, int innLevel)
{
    LOG("    CreateInn: x=%d, y=%d, innLevel=%d\n", x, y, innLevel);
    
    innLevel = (innLevel < 0) ? 0 : (innLevel > 2) ? 2 : innLevel;
    
    int minW = INN_SIZES[innLevel].minWidth;
    int maxW = INN_SIZES[innLevel].maxWidth;
    int minH = INN_SIZES[innLevel].minHeight;
    int maxH = INN_SIZES[innLevel].maxHeight;
    
    int width = minW + (rand() % (maxW - minW + 1));
    int height = minH + (rand() % (maxH - minH + 1));
    
    Building* inn = CreateBuilding(BUILDING_INN, x, y, width, height);
    if (!inn) {
        LOG("    ERROR: CreateBuilding failed for inn\n");
        return NULL;
    }
    
    LOG("    Adding inn features...\n");
    
    // Clear interior
    for (int i = 1; i < height-1; i++) {
        for (int j = 1; j < width-1; j++) {
            inn->layout[i][j] = '.';
        }
    }
    
    // ===== BAR =====
    // Bar along one wall (top)
    int barY = 2;
    for (int xPos = 3; xPos < width-3; xPos++) {
        inn->layout[barY][xPos] = 'S';
    }
    
    // Bar stools
    if (barY+1 < height-1) {
        for (int xPos = 4; xPos < width-4; xPos += 3) {
            inn->layout[barY+1][xPos] = 'C';
        }
    }
    
    // ===== TABLES =====
    int numTables = 2 + innLevel;
    int startY = barY + 3;
    
    for (int t = 0; t < numTables; t++) {
        int tableX = 4 + (t * 5);
        int tableY = startY;
        
        if (tableX < width-4 && tableY < height-4) {
            // Table
            inn->layout[tableY][tableX] = 'T';
            
            // Chairs around table
            if (tableY+1 < height-2) inn->layout[tableY+1][tableX] = 'C';
            if (tableX-1 >= 3) inn->layout[tableY][tableX-1] = 'C';
            if (tableX+1 < width-3) inn->layout[tableY][tableX+1] = 'C';
        }
    }
    
    // ===== FIREPLACE =====
    // Fireplace in corner
    inn->layout[height-3][3] = 'F';
    
    // ===== ROOMS =====
    int numRooms = 1 + innLevel;
    int roomY = height - 4;
    
    for (int r = 0; r < numRooms; r++) {
        int roomX = width - 5 - (r * 4);
        
        if (roomX >= 4) {
            // Bed
            inn->layout[roomY][roomX] = 'B';
            
            // Chest
            if (roomY+1 < height-2) {
                inn->layout[roomY+1][roomX] = 'H';
            }
        }
    }
    
    LOG("    Inn created successfully\n");
    return inn;
}

Building* CreateBlacksmith(int x, int y, int smithLevel)
{
    LOG("    CreateBlacksmith: x=%d, y=%d, smithLevel=%d\n", x, y, smithLevel);
    
    smithLevel = (smithLevel < 0) ? 0 : (smithLevel > 2) ? 2 : smithLevel;
    
    int minW = BLACKSMITH_SIZES[smithLevel].minWidth;
    int maxW = BLACKSMITH_SIZES[smithLevel].maxWidth;
    int minH = BLACKSMITH_SIZES[smithLevel].minHeight;
    int maxH = BLACKSMITH_SIZES[smithLevel].maxHeight;
    
    int width = minW + (rand() % (maxW - minW + 1));
    int height = minH + (rand() % (maxH - minH + 1));
    
    Building* smithy = CreateBuilding(BUILDING_BLACKSMITH, x, y, width, height);
    if (!smithy) {
        LOG("    ERROR: CreateBuilding failed for blacksmith\n");
        return NULL;
    }
    
    LOG("    Adding blacksmith features...\n");
    
    // Clear interior
    for (int i = 1; i < height-1; i++) {
        for (int j = 1; j < width-1; j++) {
            smithy->layout[i][j] = '.';
        }
    }
    
    // ===== FORGE =====
    // Forge in corner
    smithy->layout[3][width-3] = 'F';
    
    // ===== ANVIL =====
    // Anvil near forge
    smithy->layout[4][width-4] = 'A';
    
    // ===== SHELVES ALONG WALLS =====
    // Shelves on left wall
    for (int yPos = 2; yPos < height-2; yPos += 2) {
        smithy->layout[yPos][2] = 'S';
    }
    
    // Shelves on back wall
    for (int xPos = 2; xPos < width-4; xPos += 2) {
        smithy->layout[2][xPos] = 'S';
    }
    
    // ===== STORAGE =====
    int numStorage = 3 + smithLevel * 2;
    
    // Place barrels/crates
    for (int s = 0; s < numStorage; s++) {
        int attempts = 0;
        bool placed = false;
        
        while (!placed && attempts < 20) {
            attempts++;
            
            int storageX = 3 + (rand() % (width - 5));
            int storageY = height - 3 - (rand() % 2);
            
            if (storageX >= 2 && storageX < width-2 && storageY >= 2 && storageY < height-2) {
                if (smithy->layout[storageY][storageX] == '.') {
                    smithy->layout[storageY][storageX] = 'Q';
                    placed = true;
                }
            }
        }
    }
    
    LOG("    Blacksmith created successfully\n");
    return smithy;
}

Building* CreateGuardPost(int x, int y, int postLevel)
{
    LOG("    CreateGuardPost: x=%d, y=%d, postLevel=%d\n", x, y, postLevel);
    
    postLevel = (postLevel < 0) ? 0 : (postLevel > 2) ? 2 : postLevel;
    
    int minW = GUARD_POST_SIZES[postLevel].minWidth;
    int maxW = GUARD_POST_SIZES[postLevel].maxWidth;
    int minH = GUARD_POST_SIZES[postLevel].minHeight;
    int maxH = GUARD_POST_SIZES[postLevel].maxHeight;
    
    int width = minW + (rand() % (maxW - minW + 1));
    int height = minH + (rand() % (maxH - minH + 1));
    
    Building* post = CreateBuilding(BUILDING_GUARD_POST, x, y, width, height);
    if (!post) {
        LOG("    ERROR: CreateBuilding failed for guard post\n");
        return NULL;
    }
    
    LOG("    Adding guard post features...\n");
    
    // Clear interior
    for (int i = 1; i < height-1; i++) {
        for (int j = 1; j < width-1; j++) {
            post->layout[i][j] = '.';
        }
    }
    
    // ===== BUNKS =====
    int numBunks = 1 + postLevel;
    
    for (int b = 0; b < numBunks; b++) {
        int bunkX = 3 + (b * 3);
        if (bunkX < width-3) {
            post->layout[2][bunkX] = 'B';
        }
    }
    
    // ===== TABLE =====
    int tableX = width / 2;
    int tableY = height - 3;
    post->layout[tableY][tableX] = 'T';
    
    // Chairs at table
    if (tableX-1 >= 2) post->layout[tableY][tableX-1] = 'C';
    if (tableX+1 < width-2) post->layout[tableY][tableX+1] = 'C';
    
    // ===== WEAPON RACKS =====
    post->layout[height-3][2] = 'S';
    if (width > 5) post->layout[height-3][width-3] = 'S';
    
    // ===== STORAGE =====
    post->layout[3][width-3] = 'Q';
    
    LOG("    Guard post created successfully\n");
    return post;
}

Building* CreateTower(int x, int y, int towerLevel)
{
    LOG("    CreateTower: x=%d, y=%d, towerLevel=%d\n", x, y, towerLevel);
    
    towerLevel = (towerLevel < 0) ? 0 : (towerLevel > 2) ? 2 : towerLevel;
    LOG("    Clamped tower level: %d\n", towerLevel);
    
    int minW = TOWER_SIZES[towerLevel].minWidth;
    int maxW = TOWER_SIZES[towerLevel].maxWidth;
    int minH = TOWER_SIZES[towerLevel].minHeight;
    int maxH = TOWER_SIZES[towerLevel].maxHeight;
    
    LOG("    Size range: width=%d-%d, height=%d-%d\n", minW, maxW, minH, maxH);
    
    int widthRange = maxW - minW + 1;
    int heightRange = maxH - minH + 1;
    
    int width = minW + (rand() % widthRange);
    int height = minH + (rand() % heightRange);
    
    LOG("    Selected size: %dx%d\n", width, height);
    
    Building* tower = CreateBuilding(BUILDING_TOWER, x, y, width, height);
    if (!tower) {
        LOG("    ERROR: CreateBuilding failed for tower\n");
        return NULL;
    }
    
    LOG("    Adding tower features...\n");
    
    // Make it round-ish
    int radius = width / 2;
    int centerX = width/2;
    int centerY = height/2;
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int dx = j - centerX;
            int dy = i - centerY;
            if (dx*dx + dy*dy > radius*radius) {
                if (tower->layout[i][j] != '#') {
                    tower->layout[i][j] = '.';
                }
            }
        }
    }
    
    // Spiral staircase
    if (centerX >= 0 && centerX < width) {
        for (int i = 1; i < height-1; i++) {
            if (i >= 0 && i < height) {
                tower->layout[i][centerX] = 'S';
            }
        }
        LOG("    Placed spiral staircase at column %d\n", centerX);
    }
    
    // Wizard's study at top
    if (2 < height && centerX-1 >= 0 && centerX+1 < width) {
        if (tower->layout[2][centerX-1] == '.') tower->layout[2][centerX-1] = 'T';
        if (tower->layout[2][centerX+1] == '.') tower->layout[2][centerX+1] = 'C';
        LOG("    Placed wizard's study\n");
    }
    
    // Alchemy table at bottom
    if (height-3 >= 0 && height-3 < height) {
        if (centerX-2 >= 0) tower->layout[height-3][centerX-2] = 'A';
        if (centerX-1 >= 0) tower->layout[height-3][centerX-1] = 'Q';
        if (centerX >= 0 && centerX < width) tower->layout[height-3][centerX] = 'Q';
        LOG("    Placed alchemy table\n");
    }
    
    LOG("    Tower created successfully\n");
    return tower;
}

Building* CreateTemple(int x, int y, int templeLevel)
{
    LOG("    CreateTemple: x=%d, y=%d, templeLevel=%d\n", x, y, templeLevel);
    
    templeLevel = (templeLevel < 0) ? 0 : (templeLevel > 2) ? 2 : templeLevel;
    LOG("    Clamped temple level: %d\n", templeLevel);
    
    int minW = TEMPLE_SIZES[templeLevel].minWidth;
    int maxW = TEMPLE_SIZES[templeLevel].maxWidth;
    int minH = TEMPLE_SIZES[templeLevel].minHeight;
    int maxH = TEMPLE_SIZES[templeLevel].maxHeight;
    
    LOG("    Size range: width=%d-%d, height=%d-%d\n", minW, maxW, minH, maxH);
    
    int widthRange = maxW - minW + 1;
    int heightRange = maxH - minH + 1;
    
    int width = minW + (rand() % widthRange);
    int height = minH + (rand() % heightRange);
    
    LOG("    Selected size: %dx%d\n", width, height);
    
    Building* temple = CreateBuilding(BUILDING_TEMPLE, x, y, width, height);
    if (!temple) {
        LOG("    ERROR: CreateBuilding failed for temple\n");
        return NULL;
    }
    
    LOG("    Adding temple features...\n");
    
    // Altar
    int altarX = width/2;
    if (2 < height && altarX >= 0 && altarX < width) {
        temple->layout[2][altarX] = 'R';
        LOG("    Placed altar at (%d,%d)\n", altarX, 2);
    }
    
    // Pews
    LOG("    Placing pews\n");
    for (int i = 0; i < 4; i++) {
        int pewY = 4 + i * 2;
        if (pewY >= height) break;
        
        for (int j = 2; j < width-2; j += 2) {
            if (j >= 0 && j < width && pewY >= 0 && pewY < height) {
                if (temple->layout[pewY][j] == '.') {
                    temple->layout[pewY][j] = 'C';
                }
            }
        }
    }
    
    // Pillars
    if (3 < height && 3 < width) temple->layout[3][3] = 'P';
    if (3 < height && width-4 >= 0) temple->layout[3][width-4] = 'P';
    if (height-4 >= 0 && 3 < width) temple->layout[height-4][3] = 'P';
    if (height-4 >= 0 && width-4 >= 0) temple->layout[height-4][width-4] = 'P';
    LOG("    Placed pillars\n");
    
    // Candles
    for (int i = 1; i < height-1; i += 2) {
        if (i >= 0 && i < height) {
            if (1 < width) temple->layout[i][1] = 'F';
            if (width-2 >= 0) temple->layout[i][width-2] = 'F';
        }
    }
    LOG("    Placed candles\n");
    
    LOG("    Temple created successfully\n");
    return temple;
}

Building* CreateCastleKeep(int x, int y, int keepLevel)
{
    LOG("    CreateCastleKeep: x=%d, y=%d, keepLevel=%d\n", x, y, keepLevel);
    
    keepLevel = (keepLevel < 0) ? 0 : (keepLevel > 2) ? 2 : keepLevel;
    LOG("    Clamped keep level: %d\n", keepLevel);
    
    int minW = CASTLE_KEEP_SIZES[keepLevel].minWidth;
    int maxW = CASTLE_KEEP_SIZES[keepLevel].maxWidth;
    int minH = CASTLE_KEEP_SIZES[keepLevel].minHeight;
    int maxH = CASTLE_KEEP_SIZES[keepLevel].maxHeight;
    
    LOG("    Size range: width=%d-%d, height=%d-%d\n", minW, maxW, minH, maxH);
    
    int widthRange = maxW - minW + 1;
    int heightRange = maxH - minH + 1;
    
    int width = minW + (rand() % widthRange);
    int height = minH + (rand() % heightRange);
    
    LOG("    Selected size: %dx%d\n", width, height);
    
    Building* keep = CreateBuilding(BUILDING_CASTLE_KEEP, x, y, width, height);
    if (!keep) {
        LOG("    ERROR: CreateBuilding failed for castle keep\n");
        return NULL;
    }
    
    LOG("    Adding castle keep features...\n");
    
    // Throne room
    int throneX = width/2;
    if (3 < height && throneX >= 0 && throneX < width) {
        keep->layout[3][throneX] = 'K';
        LOG("    Placed throne at (%d,%d)\n", throneX, 3);
        
        if (throneX-1 >= 0) keep->layout[4][throneX-1] = 'C';
        if (throneX+1 < width) keep->layout[4][throneX+1] = 'C';
    }
    
    // Pillars
    LOG("    Placing pillars\n");
    for (int i = 0; i < 4; i++) {
        int pillarY = 5 + i*2;
        if (pillarY < height) {
            int pillarX1 = width/4;
            int pillarX2 = width*3/4;
            
            if (pillarX1 >= 0 && pillarX1 < width) keep->layout[pillarY][pillarX1] = 'P';
            if (pillarX2 >= 0 && pillarX2 < width) keep->layout[pillarY][pillarX2] = 'P';
        }
    }
    
    // Barracks area
    LOG("    Placing barracks\n");
    int barracksPlaced = 0;
    for (int i = 0; i < 8; i++) {
        int rangeX = 4;
        if (rangeX < 1) rangeX = 1;
        
        int bx = width-6 + (rand() % rangeX);
        int by = 2 + (rand() % 6);
        
        if (bx >= 0 && bx < width && by >= 0 && by < height) {
            if (keep->layout[by][bx] == '.') {
                keep->layout[by][bx] = 'B';
                barracksPlaced++;
            }
        }
    }
    LOG("    Placed %d barracks beds\n", barracksPlaced);
    
    // Storage
    LOG("    Placing storage\n");
    int storagePlaced = 0;
    for (int i = 0; i < 10; i++) {
        int rangeX = 6;
        int rangeY = 4;
        if (rangeX < 1) rangeX = 1;
        if (rangeY < 1) rangeY = 1;
        
        int sx = 2 + (rand() % rangeX);
        int sy = height-8 + (rand() % rangeY);
        
        if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
            if (keep->layout[sy][sx] == '.') {
                keep->layout[sy][sx] = 'Q';
                storagePlaced++;
            }
        }
    }
    LOG("    Placed %d storage items\n", storagePlaced);
    
    // Kitchen
    if (height-5 >= 0 && height-5 < height) {
        if (2 < width) keep->layout[height-5][2] = 'F';
        if (3 < width) keep->layout[height-5][3] = 'F';
        LOG("    Placed kitchen fires\n");
    }
    if (height-4 >= 0 && height-4 < height) {
        if (2 < width) keep->layout[height-4][2] = 'S';
        if (3 < width) keep->layout[height-4][3] = 'S';
        LOG("    Placed kitchen shelves\n");
    }
    
    LOG("    Castle keep created successfully\n");
    return keep;
}

Building* CreateDungeonEntrance(int x, int y, int dungeonLevel)
{
    dungeonLevel = (dungeonLevel < 0) ? 0 : (dungeonLevel > 2) ? 2 : dungeonLevel;
    
    int width = DUNGEON_ENTRANCE_SIZES[dungeonLevel].minWidth + 
                (rand() % (DUNGEON_ENTRANCE_SIZES[dungeonLevel].maxWidth - DUNGEON_ENTRANCE_SIZES[dungeonLevel].minWidth + 1));
    int height = DUNGEON_ENTRANCE_SIZES[dungeonLevel].minHeight + 
                 (rand() % (DUNGEON_ENTRANCE_SIZES[dungeonLevel].maxHeight - DUNGEON_ENTRANCE_SIZES[dungeonLevel].minHeight + 1));
    
    Building* entrance = CreateBuilding(BUILDING_DUNGEON_ENTRANCE, x, y, width, height);
    if (!entrance) return NULL;
    
    entrance->hasDoor = false;
    
    // Staircase down
    entrance->layout[height/2][width/2] = '<';
    
    // Guard posts
    entrance->layout[2][2] = 'C';
    entrance->layout[2][width-3] = 'C';
    
    // Cells
    for (int i = 0; i < 2; i++) {
        int cellX = 2 + i * (width/2);
        int cellY = height-3;
        entrance->layout[cellY][cellX] = '#';
        entrance->layout[cellY][cellX+1] = '#';
        entrance->layout[cellY-1][cellX] = '#';
        entrance->layout[cellY-1][cellX+1] = '#';
        entrance->layout[cellY-1][cellX+1] = '.';
        entrance->layout[cellY-2][cellX] = 'P';
    }
    
    return entrance;
}

Building* CreateCaveEntrance(int x, int y, int caveLevel)
{
    caveLevel = (caveLevel < 0) ? 0 : (caveLevel > 2) ? 2 : caveLevel;
    
    int width = CAVE_ENTRANCE_SIZES[caveLevel].minWidth + 
                (rand() % (CAVE_ENTRANCE_SIZES[caveLevel].maxWidth - CAVE_ENTRANCE_SIZES[caveLevel].minWidth + 1));
    int height = CAVE_ENTRANCE_SIZES[caveLevel].minHeight + 
                 (rand() % (CAVE_ENTRANCE_SIZES[caveLevel].maxHeight - CAVE_ENTRANCE_SIZES[caveLevel].minHeight + 1));
    
    Building* cave = CreateBuilding(BUILDING_CAVE_ENTRANCE, x, y, width, height);
    if (!cave) return NULL;
    
    cave->hasDoor = false;
    
    // Make it cave-like
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (i == 0 || i == height-1 || j == 0 || j == width-1) {
                if (rand() % 4 != 0) {
                    cave->layout[i][j] = '^';
                } else {
                    cave->layout[i][j] = '.';
                }
            }
        }
    }
    
    // Stalactites
    for (int i = 0; i < 5 + caveLevel * 3; i++) {
        int sx = 2 + (rand() % (width-4));
        int sy = 2 + (rand() % (height-4));
        cave->layout[sy][sx] = '^';
    }
    
    return cave;
}

Building* CreateRuin(int x, int y, int ruinLevel)
{
    ruinLevel = (ruinLevel < 0) ? 0 : (ruinLevel > 2) ? 2 : ruinLevel;
    
    int width = RUIN_SIZES[ruinLevel].minWidth + 
                (rand() % (RUIN_SIZES[ruinLevel].maxWidth - RUIN_SIZES[ruinLevel].minWidth + 1));
    int height = RUIN_SIZES[ruinLevel].minHeight + 
                 (rand() % (RUIN_SIZES[ruinLevel].maxHeight - RUIN_SIZES[ruinLevel].minHeight + 1));
    
    Building* ruin = CreateBuilding(BUILDING_RUIN, x, y, width, height);
    if (!ruin) return NULL;
    
    ruin->hasDoor = false;
    
    // Make it broken
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if ((i == 0 || i == height-1 || j == 0 || j == width-1) && rand() % 3 != 0) {
                ruin->layout[i][j] = '#';
            } else if (rand() % 10 < 3) {
                ruin->layout[i][j] = '.';
            } else {
                ruin->layout[i][j] = '#';
            }
        }
    }
    
    // Overgrown
    for (int i = 0; i < 5 + ruinLevel * 2; i++) {
        int tx = 2 + (rand() % (width-4));
        int ty = 2 + (rand() % (height-4));
        if (ruin->layout[ty][tx] == '.') {
            ruin->layout[ty][tx] = 'T';
        }
    }
    
    return ruin;
}

// ============================================================================
// TERRAIN GENERATION
// ============================================================================

// ============================================================================
// ROAD FUNCTIONS
// ============================================================================

void CreateDirtRoad(LocalMap* map, int x1, int y1, int x2, int y2)
{
    // Safety check - if start equals end, do nothing
    if (x1 == x2 && y1 == y2) return;
    
    int currentX = x1;
    int currentY = y1;
    int targetX = x2;
    int targetY = y2;
    
    int maxSteps = 200;
    int steps = 0;
    
    int pathX[200];
    int pathY[200];
    int pathLength = 0;
    pathX[pathLength] = currentX;
    pathY[pathLength] = currentY;
    pathLength++;
    
    while ((currentX != targetX || currentY != targetY) && steps < maxSteps) {
        steps++;
        
        int dx = 0, dy = 0;
        if (targetX > currentX) dx = 1;
        else if (targetX < currentX) dx = -1;
        
        if (targetY > currentY) dy = 1;
        else if (targetY < currentY) dy = -1;
        
        bool moved = false;
        
        // Try all 4 directions in order
        int directions[4][2] = {{dx, dy}, {dx, 0}, {0, dy}, {0, 0}};
        
        for (int d = 0; d < 4 && !moved; d++) {
            int ndx = directions[d][0];
            int ndy = directions[d][1];
            
            if (ndx == 0 && ndy == 0) continue;
            
            int nx = currentX + ndx;
            int ny = currentY + ndy;
            
            if (nx >= 0 && nx < map->width && ny >= 0 && ny < map->height) {
                if (IsTileWalkable(map->tiles[ny][nx])) {
                    currentX = nx;
                    currentY = ny;
                    moved = true;
                }
            }
        }
        
        if (!moved) {
            // Try any adjacent tile
            for (int ny = -1; ny <= 1 && !moved; ny++) {
                for (int nx = -1; nx <= 1 && !moved; nx++) {
                    if (nx == 0 && ny == 0) continue;
                    
                    int tx = currentX + nx;
                    int ty = currentY + ny;
                    
                    if (tx < 0 || tx >= map->width || ty < 0 || ty >= map->height) 
                        continue;
                    
                    if (!IsTileWalkable(map->tiles[ty][tx])) 
                        continue;
                    
                    // Check if already in path
                    bool inPath = false;
                    for (int p = 0; p < pathLength; p++) {
                        if (pathX[p] == tx && pathY[p] == ty) {
                            inPath = true;
                            break;
                        }
                    }
                    
                    if (!inPath) {
                        currentX = tx;
                        currentY = ty;
                        moved = true;
                    }
                }
            }
        }
        
        if (!moved) {
            break; // Stuck
        }
        
        if (pathLength < 200) {
            pathX[pathLength] = currentX;
            pathY[pathLength] = currentY;
            pathLength++;
        }
    }
    
    // Draw the road
    for (int p = 0; p < pathLength; p++) {
        int x = pathX[p];
        int y = pathY[p];
        
        if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
            if (IsTileWalkable(map->tiles[y][x])) {
                map->tiles[y][x] = ',';
            }
        }
    }
}

void CreateTownSquareWithRoads(LocalMap* map, int centerX, int centerY, int size)
{
    int half = size / 2;
    
    for (int y = centerY - half; y <= centerY + half; y++) {
        for (int x = centerX - half; x <= centerX + half; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                if (!TileIsWater(map->tiles[y][x])) map->tiles[y][x] = ',';
            }
        }
    }
    
    if (centerX >= 0 && centerX < map->width && centerY >= 0 && centerY < map->height) {
        map->tiles[centerY][centerX] = 'F';
    }
    
    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159f / 4;
        int roadLength = 80;
        
        for (int j = 1; j <= roadLength; j++) {
            int x = centerX + (int)(cos(angle) * j);
            int y = centerY + (int)(sin(angle) * j);
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                if (IsTileWalkable(map->tiles[y][x])) {
                    map->tiles[y][x] = ',';
                }
            }
        }
    }
}

// ============================================================================
// SPAWN POINT GENERATION
// ============================================================================

void GenerateVillageSpawnPoints(LocalMap* map)
{
    LOG("  Generating village spawn points...\n");
    
    if (!map) {
        LOG("  ERROR: map is NULL\n");
        return;
    }
    
    // SAFETY CHECK: If no buildings, create some basic spawn points anyway
    if (map->buildingCount == 0 || !map->buildings) {
        LOG("  WARNING: No buildings found, creating basic spawn points\n");
        
        // Create a few random spawn points so the village isn't empty
        map->spawnPointCount = 3 + (rand() % 3);
        map->spawnPoints = (SpawnPoint**)calloc(map->spawnPointCount, sizeof(SpawnPoint*));
        if (!map->spawnPoints) {
            LOG("  ERROR: Failed to allocate spawn points array\n");
            map->spawnPointCount = 0;
            return;
        }
        
        int spawnedCount = 0;
        for (int i = 0; i < map->spawnPointCount; i++) {
            SpawnPoint* spawn = (SpawnPoint*)calloc(1, sizeof(SpawnPoint));
            if (!spawn) continue;
            
            // Random position around center
            spawn->x = 100 + (rand() % 50);
            spawn->y = 100 + (rand() % 50);
            spawn->npcType = NPC_WANDERER;
            spawn->maxCount = 1;
            spawn->isUnique = false;
            
            // FIX: Use actual name from database, not "Traveler"
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
            
            spawn->type = SPAWN_TRAVELER;
            spawn->currentCount = 0;
            spawn->respawnTimer = 120;
            spawn->respawnCounter = 0;
            
            map->spawnPoints[spawnedCount] = spawn;
            spawnedCount++;
        }
        
        map->spawnPointCount = spawnedCount;
        LOG("  Generated %d fallback spawn points\n", map->spawnPointCount);
        return;
    }
    
    // Count potential spawn points from buildings
    int potentialSpawns = 0;
    for (int i = 0; i < map->buildingCount; i++) {
        Building* b = map->buildings[i];
        if (!b) {
            LOG("  WARNING: Building %d is NULL\n", i);
            continue;
        }
        if (b->type == BUILDING_HOUSE) potentialSpawns += 2;
        else if (b->type == BUILDING_SHOP) potentialSpawns += 1;
        else if (b->type == BUILDING_INN) potentialSpawns += 2;
        else if (b->type == BUILDING_BLACKSMITH) potentialSpawns += 1;
        else if (b->type == BUILDING_GUARD_POST) potentialSpawns += 3;
    }
    
    if (potentialSpawns == 0) {
        potentialSpawns = 5;
    }
    
    map->spawnPointCount = potentialSpawns;
    map->spawnPoints = (SpawnPoint**)calloc(map->spawnPointCount, sizeof(SpawnPoint*));
    if (!map->spawnPoints) {
        LOG("  ERROR: Failed to allocate spawn points array\n");
        map->spawnPointCount = 0;
        return;
    }
    
    int spawnedCount = 0;
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = (SpawnPoint*)calloc(1, sizeof(SpawnPoint));
        if (!spawn) {
            LOG("  ERROR: Failed to allocate spawn point %d\n", i);
            continue;
        }
        
        if (map->buildingCount > 0) {
            // Use different buildings for variety
            Building* b = map->buildings[i % map->buildingCount];
            if (b) {
                spawn->x = b->x + b->width / 2;
                spawn->y = b->y + b->height / 2;
                
                // FIX: Set spawn properties WITHOUT hardcoded "Villager" names
                switch (b->type) {
                    case BUILDING_HOUSE:
                        spawn->npcType = NPC_VILLAGER;
                        spawn->maxCount = 1 + (rand() % 2);
                        spawn->isUnique = (rand() % 10 < 2);
                        break;
                        
                    case BUILDING_SHOP:
                        spawn->npcType = NPC_MERCHANT;
                        spawn->maxCount = 1;
                        spawn->isUnique = true;
                        break;
                        
                    case BUILDING_INN:
                        spawn->npcType = NPC_INN_KEEPER;
                        spawn->maxCount = 2;
                        spawn->isUnique = true;
                        break;
                        
                    case BUILDING_BLACKSMITH:
                        spawn->npcType = NPC_BLACKSMITH;
                        spawn->maxCount = 1;
                        spawn->isUnique = true;
                        break;
                        
                    case BUILDING_GUARD_POST:
                        spawn->npcType = NPC_GUARD;
                        spawn->maxCount = 2 + (rand() % 2);
                        spawn->isUnique = false;
                        break;
                        
                    default:
                        spawn->npcType = NPC_VILLAGER;
                        spawn->maxCount = 1;
                        spawn->isUnique = false;
                }
                
                // FIX: Generate actual name from database
                const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
                if (spawn->isUnique) {
                    const char* last = LAST_NAMES[rand() % LAST_NAME_COUNT];
                    snprintf(spawn->npcName, 50, "%s %s", first, last);
                } else {
                    snprintf(spawn->npcName, 50, "%s", first);
                }
                
            } else {
                // Building is NULL, place randomly with proper name
                spawn->x = 50 + (rand() % 150);
                spawn->y = 50 + (rand() % 150);
                spawn->npcType = NPC_WANDERER;
                spawn->maxCount = 1;
                spawn->isUnique = false;
                
                // Generate proper name
                const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
                snprintf(spawn->npcName, 50, "%s", first);
            }
        } else {
            // No buildings, place randomly with proper name
            spawn->x = 50 + (rand() % 150);
            spawn->y = 50 + (rand() % 150);
            spawn->npcType = NPC_WANDERER;
            spawn->maxCount = 1;
            spawn->isUnique = false;
            
            // Generate proper name
            const char* first = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
            snprintf(spawn->npcName, 50, "%s", first);
        }
        
        spawn->type = SPAWN_HOUSE;
        spawn->currentCount = 0;
        spawn->respawnTimer = spawn->isUnique ? 0 : 120;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[spawnedCount] = spawn;
        spawnedCount++;
        
        LOG("  Created spawn point %d: %s at (%d,%d) type=%d\n", 
            spawnedCount, spawn->npcName, spawn->x, spawn->y, spawn->npcType);
    }
    
    map->spawnPointCount = spawnedCount;
    LOG("  Generated %d spawn points with proper names\n", map->spawnPointCount);
}

void GenerateCastleSpawnPoints(LocalMap* map)
{
    LOG("  Generating castle spawn points...\n");
    
    int potentialSpawns = 0;
    for (int i = 0; i < map->buildingCount; i++) {
        Building* b = map->buildings[i];
        if (b->type == BUILDING_CASTLE_KEEP) potentialSpawns += 5;
        else if (b->type == BUILDING_GUARD_POST) potentialSpawns += 3;
        else if (b->type == BUILDING_FORT_BARRACKS) potentialSpawns += 4;
        else if (b->type == BUILDING_BLACKSMITH) potentialSpawns += 1;
        else if (b->type == BUILDING_HOUSE) potentialSpawns += 1;
    }
    
    map->spawnPointCount = potentialSpawns;
    map->spawnPoints = (SpawnPoint**)malloc(map->spawnPointCount * sizeof(SpawnPoint*));
    
    int spawnIndex = 0;
    
    for (int i = 0; i < map->buildingCount; i++) {
        Building* b = map->buildings[i];
        int numSpawns = 0;
        NPCType npcType = NPC_VILLAGER;
        int maxCount = 1;
        bool isUnique = false;
        const char* baseName = "";
        
        switch (b->type) {
            case BUILDING_CASTLE_KEEP:
                numSpawns = 5;
                npcType = NPC_GUARD;
                maxCount = 2;
                isUnique = true;
                baseName = "Royal Guard";
                break;
                
            case BUILDING_GUARD_POST:
                numSpawns = 3;
                npcType = NPC_GUARD;
                maxCount = 2 + (rand() % 2);
                isUnique = false;
                baseName = "Castle Guard";
                break;
                
            case BUILDING_FORT_BARRACKS:
                numSpawns = 4;
                npcType = NPC_GUARD;
                maxCount = 3 + (rand() % 2);
                isUnique = false;
                baseName = "Soldier";
                break;
                
            case BUILDING_BLACKSMITH:
                numSpawns = 1;
                npcType = NPC_BLACKSMITH;
                maxCount = 1;
                isUnique = true;
                baseName = "Armorer";
                break;
                
            case BUILDING_HOUSE:
                numSpawns = 1;
                npcType = NPC_VILLAGER;
                maxCount = 1 + (rand() % 2);
                isUnique = false;
                baseName = "Servant";
                break;
                
            default:
                continue;
        }
        
        for (int j = 0; j < numSpawns && spawnIndex < map->spawnPointCount; j++) {
            SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
            memset(spawn, 0, sizeof(SpawnPoint));
            
            spawn->x = b->x + (rand() % b->width);
            spawn->y = b->y + (rand() % b->height);
            spawn->type = SPAWN_GUARD_POST;
            spawn->npcType = npcType;
            spawn->maxCount = maxCount;
            spawn->isUnique = isUnique;
            
            if (isUnique) {
                snprintf(spawn->npcName, 50, "%s %d", baseName, spawnIndex);
            } else {
                snprintf(spawn->npcName, 50, "%s", baseName);
            }
            
            spawn->currentCount = 0;
            spawn->respawnTimer = isUnique ? 0 : 180;
            spawn->respawnCounter = 0;
            
            map->spawnPoints[spawnIndex++] = spawn;
        }
    }
    
    LOG("  Generated %d castle spawn points\n", spawnIndex);
    map->spawnPointCount = spawnIndex;
}

void GenerateCaveSpawnPoints(LocalMap* map)
{
    LOG("  Generating cave spawn points...\n");
    
    int numSpawns = 5 + (rand() % 8);
    map->spawnPointCount = numSpawns;
    map->spawnPoints = (SpawnPoint**)malloc(numSpawns * sizeof(SpawnPoint*));
    
    for (int i = 0; i < numSpawns; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        if (map->buildingCount > 0) {
            Building* b = map->buildings[0];
            spawn->x = b->x + (rand() % 20) - 10;
            spawn->y = b->y + (rand() % 20) - 10;
        } else {
            spawn->x = 50 + (rand() % 150);
            spawn->y = 50 + (rand() % 150);
        }
        
        int creatureRoll = rand() % 100;
        if (creatureRoll < 40) {
            spawn->npcType = NPC_BANDIT;
            snprintf(spawn->npcName, 50, "Goblin");
            spawn->maxCount = 3 + (rand() % 4);
        } else if (creatureRoll < 70) {
            spawn->npcType = NPC_BANDIT;
            snprintf(spawn->npcName, 50, "Kobold");
            spawn->maxCount = 4 + (rand() % 6);
        } else {
            spawn->npcType = NPC_BANDIT;
            snprintf(spawn->npcName, 50, "Bat");
            spawn->maxCount = 5 + (rand() % 8);
        }
        
        spawn->type = SPAWN_BANDIT_CAMP;
        spawn->isUnique = false;
        spawn->currentCount = 0;
        spawn->respawnTimer = 180;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
    
    LOG("  Generated %d cave spawn points\n", numSpawns);
}

void GenerateTowerSpawnPoints(LocalMap* map)
{
    LOG("  Generating tower spawn points...\n");
    
    int numSpawns = 3 + (rand() % 5);
    map->spawnPointCount = numSpawns;
    map->spawnPoints = (SpawnPoint**)malloc(numSpawns * sizeof(SpawnPoint*));
    
    for (int i = 0; i < numSpawns; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        Building* tower = NULL;
        for (int j = 0; j < map->buildingCount; j++) {
            if (map->buildings[j]->type == BUILDING_TOWER) {
                tower = map->buildings[j];
                break;
            }
        }
        
        if (tower) {
            spawn->x = tower->x + (rand() % tower->width);
            spawn->y = tower->y + (rand() % tower->height);
        } else {
            spawn->x = 50 + (rand() % 150);
            spawn->y = 50 + (rand() % 150);
        }
        
        if (i == 0) {
            spawn->npcType = NPC_WIZARD;
            spawn->isUnique = true;
            spawn->maxCount = 1;
            snprintf(spawn->npcName, 50, "Wizard");
            spawn->respawnTimer = 0;
        } else {
            spawn->npcType = NPC_VILLAGER;
            spawn->isUnique = false;
            spawn->maxCount = 1 + (rand() % 2);
            
            if (rand() % 3 == 0) {
                snprintf(spawn->npcName, 50, "Apprentice");
            } else {
                const char* creatures[] = {"Golem", "Familiar", "Elemental", "Imp"};
                snprintf(spawn->npcName, 50, creatures[rand() % 4]);
                spawn->npcType = NPC_BANDIT;
            }
            spawn->respawnTimer = 240;
        }
        
        spawn->type = SPAWN_HOUSE;
        spawn->currentCount = 0;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
    
    LOG("  Generated %d tower spawn points\n", numSpawns);
}

void GenerateDungeonSpawnPoints(LocalMap* map)
{
    LOG("  Generating dungeon spawn points...\n");
    
    int numSpawns = 8 + (rand() % 8);
    map->spawnPointCount = numSpawns;
    map->spawnPoints = (SpawnPoint**)malloc(numSpawns * sizeof(SpawnPoint*));
    
    Building* entrance = NULL;
    for (int i = 0; i < map->buildingCount; i++) {
        if (map->buildings[i]->type == BUILDING_DUNGEON_ENTRANCE) {
            entrance = map->buildings[i];
            break;
        }
    }
    
    for (int i = 0; i < numSpawns; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        if (entrance) {
            spawn->x = entrance->x + (rand() % 30) - 15;
            spawn->y = entrance->y + (rand() % 30) - 15;
        } else {
            spawn->x = 50 + (rand() % 150);
            spawn->y = 50 + (rand() % 150);
        }
        
        if (i == 0) {
            spawn->npcType = NPC_BANDIT;
            spawn->isUnique = true;
            spawn->maxCount = 1;
            snprintf(spawn->npcName, 50, "Dungeon Lord");
            spawn->respawnTimer = 0;
        } else if (i < 4) {
            spawn->npcType = NPC_GUARD;
            spawn->isUnique = false;
            spawn->maxCount = 2 + (rand() % 3);
            snprintf(spawn->npcName, 50, "Prison Guard");
            spawn->respawnTimer = 120;
        } else if (i < 8) {
            spawn->npcType = NPC_BANDIT;
            spawn->isUnique = false;
            spawn->maxCount = 3 + (rand() % 4);
            if (rand() % 2 == 0) {
                snprintf(spawn->npcName, 50, "Skeleton");
            } else {
                snprintf(spawn->npcName, 50, "Zombie");
            }
            spawn->respawnTimer = 180;
        } else {
            spawn->npcType = NPC_VILLAGER;
            spawn->isUnique = false;
            spawn->maxCount = 1;
            snprintf(spawn->npcName, 50, "Prisoner");
            spawn->respawnTimer = 0;
        }
        
        spawn->type = SPAWN_BANDIT_CAMP;
        spawn->currentCount = 0;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
    
    LOG("  Generated %d dungeon spawn points\n", numSpawns);
}

void GenerateRuinsSpawnPoints(LocalMap* map)
{
    LOG("  Generating ruins spawn points...\n");
    
    int numSpawns = 4 + (rand() % 6);
    map->spawnPointCount = numSpawns;
    map->spawnPoints = (SpawnPoint**)malloc(numSpawns * sizeof(SpawnPoint*));
    
    for (int i = 0; i < numSpawns; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        if (map->buildingCount > 0) {
            Building* b = map->buildings[i % map->buildingCount];
            spawn->x = b->x + b->width / 2;
            spawn->y = b->y + b->height / 2;
        } else {
            spawn->x = 50 + (rand() % 150);
            spawn->y = 50 + (rand() % 150);
        }
        
        int typeRoll = rand() % 100;
        if (typeRoll < 50) {
            spawn->npcType = NPC_BANDIT;
            snprintf(spawn->npcName, 50, "Grave Robber");
            spawn->maxCount = 2 + (rand() % 4);
            spawn->respawnTimer = 150;
        } else if (typeRoll < 75) {
            spawn->npcType = NPC_WANDERER;
            if (rand() % 4 == 0) {
                spawn->isUnique = true;
                snprintf(spawn->npcName, 50, "Explorer");
                spawn->maxCount = 1;
                spawn->respawnTimer = 0;
            } else {
                spawn->isUnique = false;
                snprintf(spawn->npcName, 50, "Explorer");
                spawn->maxCount = 1 + (rand() % 2);
                spawn->respawnTimer = 200;
            }
        } else {
            spawn->npcType = NPC_VILLAGER;
            if (rand() % 3 == 0) {
                spawn->isUnique = true;
                snprintf(spawn->npcName, 50, "Hermit");
                spawn->maxCount = 1;
                spawn->respawnTimer = 0;
            } else {
                spawn->isUnique = false;
                snprintf(spawn->npcName, 50, "Hermit");
                spawn->maxCount = 1;
                spawn->respawnTimer = 250;
            }
        }
        
        spawn->type = SPAWN_BANDIT_CAMP;
        spawn->isUnique = spawn->isUnique;
        spawn->currentCount = 0;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
    
    LOG("  Generated %d ruins spawn points\n", numSpawns);
}

void GenerateFortSpawnPoints(LocalMap* map)
{
    LOG("  Generating fort spawn points...\n");
    
    int potentialSpawns = 0;
    for (int i = 0; i < map->buildingCount; i++) {
        Building* b = map->buildings[i];
        if (b->type == BUILDING_FORT_BARRACKS) potentialSpawns += 4;
        else if (b->type == BUILDING_GUARD_POST) potentialSpawns += 2;
        else if (b->type == BUILDING_BLACKSMITH) potentialSpawns += 1;
    }
    
    map->spawnPointCount = potentialSpawns;
    map->spawnPoints = (SpawnPoint**)malloc(potentialSpawns * sizeof(SpawnPoint*));
    
    int spawnIndex = 0;
    
    for (int i = 0; i < map->buildingCount; i++) {
        Building* b = map->buildings[i];
        int numSpawns = 0;
        NPCType npcType = NPC_GUARD;
        int maxCount = 1;
        const char* baseName = "";
        
        switch (b->type) {
            case BUILDING_FORT_BARRACKS:
                numSpawns = 4;
                npcType = NPC_GUARD;
                maxCount = 3 + (rand() % 3);
                baseName = "Soldier";
                break;
                
            case BUILDING_GUARD_POST:
                numSpawns = 2;
                npcType = NPC_GUARD;
                maxCount = 2 + (rand() % 2);
                baseName = "Sentry";
                break;
                
            case BUILDING_BLACKSMITH:
                numSpawns = 1;
                npcType = NPC_BLACKSMITH;
                maxCount = 1;
                baseName = "Armorer";
                break;
                
            default:
                continue;
        }
        
        for (int j = 0; j < numSpawns && spawnIndex < map->spawnPointCount; j++) {
            SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
            memset(spawn, 0, sizeof(SpawnPoint));
            
            spawn->x = b->x + (rand() % b->width);
            spawn->y = b->y + (rand() % b->height);
            spawn->type = SPAWN_GUARD_POST;
            spawn->npcType = npcType;
            spawn->maxCount = maxCount;
            spawn->isUnique = (j == 0 && b->type == BUILDING_FORT_BARRACKS);
            
            if (spawn->isUnique) {
                snprintf(spawn->npcName, 50, "Sergeant");
            } else {
                snprintf(spawn->npcName, 50, "%s", baseName);
            }
            
            spawn->currentCount = 0;
            spawn->respawnTimer = spawn->isUnique ? 0 : 150;
            spawn->respawnCounter = 0;
            
            map->spawnPoints[spawnIndex++] = spawn;
        }
    }
    
    LOG("  Generated %d fort spawn points\n", spawnIndex);
    map->spawnPointCount = spawnIndex;
}

void GenerateBanditCampSpawnPoints(LocalMap* map)
{
    LOG("  Generating bandit camp spawn points...\n");
    
    int numSpawns = 3 + (rand() % 4);
    map->spawnPointCount = numSpawns;
    map->spawnPoints = (SpawnPoint**)malloc(numSpawns * sizeof(SpawnPoint*));
    
    for (int i = 0; i < numSpawns; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = 50 + (rand() % 150);
        spawn->y = 50 + (rand() % 150);
        
        if (i == 0 && rand() % 2 == 0) {
            spawn->isUnique = true;
            spawn->npcType = NPC_BANDIT;
            spawn->maxCount = 1;
            snprintf(spawn->npcName, 50, "Bandit Leader");
            spawn->respawnTimer = 0;
        } else {
            spawn->isUnique = false;
            spawn->npcType = NPC_BANDIT;
            spawn->maxCount = 2 + (rand() % 3);
            snprintf(spawn->npcName, 50, "Bandit");
            spawn->respawnTimer = 120;
        }
        
        spawn->type = SPAWN_BANDIT_CAMP;
        spawn->currentCount = 0;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
    
    LOG("  Generated %d bandit camp spawn points\n", numSpawns);
}

void GenerateTravelerSpawnPoints(LocalMap* map)
{
    LOG("  Generating traveler spawn points...\n");
    
    int numSpawns = 2 + (rand() % 4);
    map->spawnPointCount = numSpawns;
    map->spawnPoints = (SpawnPoint**)malloc(numSpawns * sizeof(SpawnPoint*));
    
    for (int i = 0; i < numSpawns; i++) {
        SpawnPoint* spawn = (SpawnPoint*)malloc(sizeof(SpawnPoint));
        memset(spawn, 0, sizeof(SpawnPoint));
        
        spawn->x = 50 + (rand() % 150);
        spawn->y = 50 + (rand() % 150);
        spawn->type = SPAWN_TRAVELER;
        
        if (rand() % 3 == 0) {
            spawn->npcType = NPC_BANDIT;
            snprintf(spawn->npcName, 50, "Highwayman");
            spawn->maxCount = 1 + (rand() % 2);
        } else {
            spawn->npcType = NPC_WANDERER;
            snprintf(spawn->npcName, 50, "Traveler");
            spawn->maxCount = 1;
        }
        
        spawn->isUnique = false;
        spawn->currentCount = 0;
        spawn->respawnTimer = 180;
        spawn->respawnCounter = 0;
        
        map->spawnPoints[i] = spawn;
    }
    
    LOG("  Generated %d traveler spawn points\n", numSpawns);
}

void GenerateSpawnPoints(LocalMap* map, LocationType locationType)
{
    switch (locationType) {
        case LOCATION_VILLAGE:
            GenerateVillageSpawnPoints(map);
            break;
        case LOCATION_CASTLE:
            GenerateCastleSpawnPoints(map);
            break;
        case LOCATION_CAVE:
            GenerateCaveSpawnPoints(map);
            break;
        case LOCATION_WIZARD_TOWER:
            GenerateTowerSpawnPoints(map);
            break;
        case LOCATION_DUNGEON:
            GenerateDungeonSpawnPoints(map);
            break;
        case LOCATION_RUINS:
            GenerateRuinsSpawnPoints(map);
            break;
        case LOCATION_FORT:
            GenerateFortSpawnPoints(map);
            break;
        default:
            if (map->buildings && map->buildingCount > 0) {
                GenerateVillageSpawnPoints(map);
            }
            break;
    }
}

// ============================================================================
// STRUCTURE PLACEMENT
// ============================================================================

void PlaceVillage(LocalMap* map)
{
    LOG("=== PLACE VILLAGE START ===\n");
    
    if (!map) {
        LOG("ERROR: map is NULL in PlaceVillage\n");
        return;
    }
    
    if (!map->tiles) {
        LOG("ERROR: map->tiles is NULL in PlaceVillage\n");
        return;
    }
    
    LOG("Map dimensions: %dx%d\n", map->width, map->height);
    
    int centerX = LOCAL_MAP_WIDTH / 2;
    int centerY = LOCAL_MAP_HEIGHT / 2;
    
    LOG("Center coordinates: (%d,%d)\n", centerX, centerY);
    
    // Validate center coordinates
    if (centerX < 0 || centerX >= map->width || centerY < 0 || centerY >= map->height) {
        LOG("ERROR: Invalid center coordinates (%d,%d) for map size %dx%d\n", 
            centerX, centerY, map->width, map->height);
        return;
    }
    
    // Create a simple town square (just clear an area, no roads)
    LOG("Creating town square at center...\n");
    int squareSize = 15; // Smaller square to leave room for buildings
    int half = squareSize / 2;
    
    if (half <= 0) {
        LOG("ERROR: squareSize/2 = 0, using half=1\n");
        half = 1;
    }
    
    for (int y = centerY - half; y <= centerY + half; y++) {
        for (int x = centerX - half; x <= centerX + half; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                if (!TileIsWater(map->tiles[y][x])) map->tiles[y][x] = ','; // Dirt path for town square
            }
        }
    }
    
    // Place fountain in center
    if (centerX >= 0 && centerX < map->width && centerY >= 0 && centerY < map->height) {
        map->tiles[centerY][centerX] = 'F';
        LOG("Placed fountain at (%d,%d)\n", centerX, centerY);
    }
    
    // Determine village size and building counts
    int villageSize = 1 + (rand() % 3); // 1-3
    LOG("Village size: %d\n", villageSize);
    
    int numHouses, numShops, numInns, numBlacksmiths, numGuardPosts;
    
    switch(villageSize) {
        case 1: // Small village
            numHouses = 6;
            numShops = 1;
            numInns = 0;
            numBlacksmiths = 0;
            numGuardPosts = 1;
            break;
        case 2: // Medium village
            numHouses = 10;
            numShops = 2;
            numInns = 1;
            numBlacksmiths = 1;
            numGuardPosts = 2;
            break;
        case 3: // Large village
            numHouses = 14;
            numShops = 3;
            numInns = 1;
            numBlacksmiths = 1;
            numGuardPosts = 3;
            break;
        default:
            numHouses = 8;
            numShops = 1;
            numInns = 0;
            numBlacksmiths = 0;
            numGuardPosts = 1;
    }
    
    LOG("Building targets: houses=%d, shops=%d, inns=%d, blacksmiths=%d, guards=%d\n", 
        numHouses, numShops, numInns, numBlacksmiths, numGuardPosts);
    
    // Store building positions (for logging only, no roads)
    int totalPlaced = 0;
    
    // ===== GRID-BASED HOUSE PLACEMENT =====
    LOG("Placing houses in grid pattern...\n");
    
    // Grid parameters - fixed positions to avoid random failures
    int gridStartX = centerX - 80;
    int gridStartY = centerY - 70;
    int gridSpacing = 30;
    int housesPerRow = 4;
    
    // Clamp grid start to safe bounds
    if (gridStartX < 20) gridStartX = 20;
    if (gridStartY < 20) gridStartY = 20;
    if (gridStartX + (housesPerRow * gridSpacing) > map->width - 50) {
        gridStartX = map->width - 50 - (housesPerRow * gridSpacing);
    }
    
    LOG("Grid start: (%d,%d), spacing: %d\n", gridStartX, gridStartY, gridSpacing);
    
    int housesPlaced = 0;
    int maxHouseAttempts = numHouses * 5;
    int houseAttempts = 0;
    
    for (int i = 0; i < numHouses && houseAttempts < maxHouseAttempts; i++) {
        houseAttempts++;
        
        // Calculate grid position
        int row = housesPlaced / housesPerRow;
        int col = housesPlaced % housesPerRow;
        
        int x = gridStartX + (col * gridSpacing) + (rand() % 6) - 3; // Small random offset
        int y = gridStartY + (row * gridSpacing) + (rand() % 6) - 3;
        
        // Safety bounds
        if (x < 20) x = 20;
        if (x + 14 > map->width - 20) x = map->width - 20 - 14;
        if (y < 20) y = 20;
        if (y + 10 > map->height - 20) y = map->height - 20 - 10;
        
        LOG("  Attempting house %d at grid (%d,%d) -> (%d,%d)\n", i+1, col, row, x, y);
        
        if (IsAreaFree(map, x, y, 14, 10)) {
            // Determine wealth level based on distance from center
            int wealthLevel;
            int distFromCenter = abs(x - centerX) + abs(y - centerY);
            if (distFromCenter > 100) wealthLevel = 0;      // Poor on outskirts
            else if (distFromCenter > 60) wealthLevel = 1;  // Middle class
            else wealthLevel = 2;                            // Wealthy near center
            
            LOG("    Area free, wealth level: %d\n", wealthLevel);
            
            Building* house = CreateHouse(x, y, wealthLevel);
            if (house) {
                DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
                SetBuildingDoor(house, doorPos);
                PlaceBuildingOnMap(map, house);
                
                // Add to buildings array
                map->buildingCount++;
                map->buildings = (Building**)realloc(map->buildings, 
                                   map->buildingCount * sizeof(Building*));
                if (map->buildings) {
                    map->buildings[map->buildingCount - 1] = house;
                    housesPlaced++;
                    totalPlaced++;
                    LOG("    House placed successfully. Total houses: %d\n", housesPlaced);
                } else {
                    LOG("    ERROR: Failed to realloc buildings array\n");
                    free(house);
                }
            } else {
                LOG("    ERROR: Failed to create house\n");
            }
        } else {
            LOG("    Area not free at (%d,%d)\n", x, y);
            // Try a different position for this house
            i--; // Try this house again
        }
    }
    
    LOG("Houses placed: %d/%d\n", housesPlaced, numHouses);
    
    // If we couldn't place enough houses, add some in fixed positions
    if (housesPlaced < numHouses) {
        LOG("WARNING: Not enough houses placed, adding fixed positions\n");
        
        int fixedPositions[][2] = {
            {centerX - 60, centerY - 50},
            {centerX + 50, centerY - 50},
            {centerX - 60, centerY + 40},
            {centerX + 50, centerY + 40},
            {centerX - 70, centerY},
            {centerX + 60, centerY},
            {centerX, centerY - 60},
            {centerX, centerY + 50}
        };
        
        for (int i = 0; i < 8 && housesPlaced < numHouses; i++) {
            int x = fixedPositions[i][0];
            int y = fixedPositions[i][1];
            
            // Safety bounds
            if (x < 20) x = 20;
            if (x + 14 > map->width - 20) x = map->width - 20 - 14;
            if (y < 20) y = 20;
            if (y + 10 > map->height - 20) y = map->height - 20 - 10;
            
            LOG("  Trying fixed position (%d,%d)\n", x, y);
            
            if (IsAreaFree(map, x, y, 14, 10)) {
                Building* house = CreateHouse(x, y, 1); // Middle class
                if (house) {
                    DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
                    SetBuildingDoor(house, doorPos);
                    PlaceBuildingOnMap(map, house);
                    
                    map->buildingCount++;
                    map->buildings = (Building**)realloc(map->buildings, 
                                       map->buildingCount * sizeof(Building*));
                    if (map->buildings) {
                        map->buildings[map->buildingCount - 1] = house;
                        housesPlaced++;
                        totalPlaced++;
                        LOG("    Fixed house placed\n");
                    } else {
                        free(house);
                    }
                }
            }
        }
    }
    
    // ===== PLACE SHOPS =====
    LOG("Placing shops...\n");
    int shopsPlaced = 0;
    int shopAttempts = 0;
    int maxShopAttempts = numShops * 10;
    
    for (int i = 0; i < numShops && shopAttempts < maxShopAttempts; i++) {
        shopAttempts++;
        
        // Place shops in a ring around the center
        float angle = (float)((i * 90) + (rand() % 30));
        while (angle > 360) angle -= 360;
        
        float rad = angle * 3.14159f / 180.0f;
        int distance = 50 + (rand() % 30);
        
        int x = centerX + (int)(cos(rad) * distance);
        int y = centerY + (int)(sin(rad) * distance);
        
        // Safety bounds
        if (x < 20) x = 20;
        if (x + 14 > map->width - 20) x = map->width - 20 - 14;
        if (y < 20) y = 20;
        if (y + 8 > map->height - 20) y = map->height - 20 - 8;
        
        LOG("  Attempting shop at (%d,%d)\n", x, y);
        
        if (IsAreaFree(map, x, y, 14, 8)) {
            int shopLevel = rand() % 3; // 0-2
            Building* shop = CreateShop(x, y, shopLevel);
            if (shop) {
                DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
                SetBuildingDoor(shop, doorPos);
                PlaceBuildingOnMap(map, shop);
                
                map->buildingCount++;
                map->buildings = (Building**)realloc(map->buildings, 
                                   map->buildingCount * sizeof(Building*));
                if (map->buildings) {
                    map->buildings[map->buildingCount - 1] = shop;
                    shopsPlaced++;
                    totalPlaced++;
                    LOG("    Shop placed\n");
                } else {
                    free(shop);
                }
            }
        } else {
            LOG("    Area not free\n");
            i--; // Try again
        }
    }
    
    LOG("Shops placed: %d/%d\n", shopsPlaced, numShops);
    
    // ===== PLACE INN =====
    if (numInns > 0) {
        LOG("Placing inn...\n");
        int innAttempts = 0;
        bool innPlaced = false;
        
        while (!innPlaced && innAttempts < 30) {
            innAttempts++;
            
            float angle = (float)(rand() % 360);
            float rad = angle * 3.14159f / 180.0f;
            int distance = 60 + (rand() % 30);
            
            int x = centerX + (int)(cos(rad) * distance);
            int y = centerY + (int)(sin(rad) * distance);
            
            if (x < 20) x = 20;
            if (x + 18 > map->width - 20) x = map->width - 20 - 18;
            if (y < 20) y = 20;
            if (y + 10 > map->height - 20) y = map->height - 20 - 10;
            
            LOG("  Attempting inn at (%d,%d)\n", x, y);
            
            if (IsAreaFree(map, x, y, 18, 10)) {
                int innLevel = rand() % 3;
                Building* inn = CreateInn(x, y, innLevel);
                if (inn) {
                    DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
                    SetBuildingDoor(inn, doorPos);
                    PlaceBuildingOnMap(map, inn);
                    
                    map->buildingCount++;
                    map->buildings = (Building**)realloc(map->buildings, 
                                       map->buildingCount * sizeof(Building*));
                    if (map->buildings) {
                        map->buildings[map->buildingCount - 1] = inn;
                        innPlaced = true;
                        totalPlaced++;
                        LOG("    Inn placed\n");
                    } else {
                        free(inn);
                    }
                }
            }
        }
        
        if (innPlaced) {
            LOG("Inn placed successfully\n");
        } else {
            LOG("WARNING: Could not place inn\n");
        }
    }
    
    // ===== PLACE BLACKSMITH =====
    if (numBlacksmiths > 0) {
        LOG("Placing blacksmith...\n");
        int smithAttempts = 0;
        bool smithPlaced = false;
        
        while (!smithPlaced && smithAttempts < 30) {
            smithAttempts++;
            
            float angle = (float)(rand() % 360);
            float rad = angle * 3.14159f / 180.0f;
            int distance = 55 + (rand() % 30);
            
            int x = centerX + (int)(cos(rad) * distance);
            int y = centerY + (int)(sin(rad) * distance);
            
            if (x < 20) x = 20;
            if (x + 16 > map->width - 20) x = map->width - 20 - 16;
            if (y < 20) y = 20;
            if (y + 9 > map->height - 20) y = map->height - 20 - 9;
            
            LOG("  Attempting blacksmith at (%d,%d)\n", x, y);
            
            if (IsAreaFree(map, x, y, 16, 9)) {
                int smithLevel = rand() % 3;
                Building* smith = CreateBlacksmith(x, y, smithLevel);
                if (smith) {
                    DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
                    SetBuildingDoor(smith, doorPos);
                    PlaceBuildingOnMap(map, smith);
                    
                    map->buildingCount++;
                    map->buildings = (Building**)realloc(map->buildings, 
                                       map->buildingCount * sizeof(Building*));
                    if (map->buildings) {
                        map->buildings[map->buildingCount - 1] = smith;
                        smithPlaced = true;
                        totalPlaced++;
                        LOG("    Blacksmith placed\n");
                    } else {
                        free(smith);
                    }
                }
            }
        }
        
        if (smithPlaced) {
            LOG("Blacksmith placed successfully\n");
        } else {
            LOG("WARNING: Could not place blacksmith\n");
        }
    }
    
    // ===== PLACE GUARD POSTS =====
    LOG("Placing guard posts...\n");
    int guardsPlaced = 0;
    int guardAttempts = 0;
    
    for (int i = 0; i < numGuardPosts && guardAttempts < 50; i++) {
        guardAttempts++;
        
        int x, y;
        switch(i) {
            case 0: // North
                x = centerX - 15;
                y = centerY - 80;
                break;
            case 1: // South
                x = centerX - 15;
                y = centerY + 70;
                break;
            case 2: // East
                x = centerX + 70;
                y = centerY - 15;
                break;
            case 3: // West
                x = centerX - 80;
                y = centerY - 15;
                break;
            default:
                x = centerX + (rand() % 100) - 50;
                y = centerY + (rand() % 100) - 50;
        }
        
        // Clamp to safe bounds
        if (x < 20) x = 20;
        if (x + 12 > map->width - 20) x = map->width - 20 - 12;
        if (y < 20) y = 20;
        if (y + 7 > map->height - 20) y = map->height - 20 - 7;
        
        LOG("  Attempting guard post %d at (%d,%d)\n", i+1, x, y);
        
        if (IsAreaFree(map, x, y, 12, 7)) {
            int postLevel = rand() % 3;
            Building* post = CreateGuardPost(x, y, postLevel);
            if (post) {
                DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
                SetBuildingDoor(post, doorPos);
                PlaceBuildingOnMap(map, post);
                
                map->buildingCount++;
                map->buildings = (Building**)realloc(map->buildings, 
                                   map->buildingCount * sizeof(Building*));
                if (map->buildings) {
                    map->buildings[map->buildingCount - 1] = post;
                    guardsPlaced++;
                    totalPlaced++;
                    LOG("    Guard post placed\n");
                } else {
                    free(post);
                }
            }
        } else {
            LOG("    Area not free\n");
            i--; // Try again
        }
    }
    
    LOG("Guard posts placed: %d/%d\n", guardsPlaced, numGuardPosts);
    
    // ===== FINAL STATISTICS =====
    LOG("=== VILLAGE PLACEMENT COMPLETE ===\n");
    LOG("Total buildings placed: %d\n", totalPlaced);
    LOG("Final building count in map: %d\n", map->buildingCount);
    LOG("================================\n");
    
    // NO ROADS - Intentionally omitted
}

void PlaceCastle(LocalMap* map)
{
    if (!map) return;
    
    LOG("  Placing castle structures...\n");
    
    int centerX = LOCAL_MAP_WIDTH / 2;
    int centerY = LOCAL_MAP_HEIGHT / 2;
    
    for (int y = centerY - 50; y <= centerY + 50; y++) {
        for (int x = centerX - 50; x <= centerX + 50; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                ClearTileKeepWater(map, x, y);
            }
        }
    }
    
    // Main keep
    Building* keep = CreateCastleKeep(centerX - 15, centerY - 12, 2);
    if (keep) {
        DoorPosition doorPos = GetDoorPositionForBuilding(centerX - 15, centerY - 12, centerX, centerY);
        SetBuildingDoor(keep, doorPos);
        PlaceBuildingOnMap(map, keep);
        map->buildingCount++;
        map->buildings = (Building**)realloc(map->buildings, 
                           map->buildingCount * sizeof(Building*));
        map->buildings[map->buildingCount - 1] = keep;
    }
    
    // Guard towers
    for (int i = 0; i < 4; i++) {
        float angle = i * 3.14159f / 2;
        int x = centerX + (int)(cos(angle) * 60);
        int y = centerY + (int)(sin(angle) * 60);
        
        Building* tower = CreateTower(x, y, 1);
        if (tower) {
            DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
            SetBuildingDoor(tower, doorPos);
            PlaceBuildingOnMap(map, tower);
            map->buildingCount++;
            map->buildings = (Building**)realloc(map->buildings, 
                               map->buildingCount * sizeof(Building*));
            map->buildings[map->buildingCount - 1] = tower;
        }
    }
    
    // Barracks
    Building* barracks = CreateGuardPost(centerX - 40, centerY + 20, 2);
    if (barracks) {
        DoorPosition doorPos = GetDoorPositionForBuilding(centerX - 40, centerY + 20, centerX, centerY);
        SetBuildingDoor(barracks, doorPos);
        PlaceBuildingOnMap(map, barracks);
        map->buildingCount++;
        map->buildings = (Building**)realloc(map->buildings, 
                           map->buildingCount * sizeof(Building*));
        map->buildings[map->buildingCount - 1] = barracks;
    }
    
    // Blacksmith
    Building* smith = CreateBlacksmith(centerX + 35, centerY - 20, 1);
    if (smith) {
        DoorPosition doorPos = GetDoorPositionForBuilding(centerX + 35, centerY - 20, centerX, centerY);
        SetBuildingDoor(smith, doorPos);
        PlaceBuildingOnMap(map, smith);
        map->buildingCount++;
        map->buildings = (Building**)realloc(map->buildings, 
                           map->buildingCount * sizeof(Building*));
        map->buildings[map->buildingCount - 1] = smith;
    }
    
    LOG("  Castle placed with %d buildings\n", map->buildingCount);
}

void PlaceCave(LocalMap* map)
{
    if (!map) return;
    
    LOG("  Placing cave structures...\n");
    
    int centerX = LOCAL_MAP_WIDTH / 2;
    int centerY = LOCAL_MAP_HEIGHT / 2;
    
    for (int y = centerY - 60; y <= centerY + 60; y++) {
        for (int x = centerX - 60; x <= centerX + 60; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                // terrain is already a hilly clearing (LocalTerrain); just scatter a few boulders
                char& t = map->tiles[y][x];
                if (rand() % 100 < 7 && TileWalkable(t) && !TileIsWater(t) && t != TILE_ROAD) t = TILE_BOULDER;
            }
        }
    }
    
    Building* cave = CreateCaveEntrance(centerX - 15, centerY - 10, 2);
    if (cave) {
        DoorPosition doorPos = GetDoorPositionForBuilding(centerX - 15, centerY - 10, centerX, centerY);
        SetBuildingDoor(cave, doorPos);
        PlaceBuildingOnMap(map, cave);
        map->buildingCount++;
        map->buildings = (Building**)realloc(map->buildings, 
                           map->buildingCount * sizeof(Building*));
        map->buildings[map->buildingCount - 1] = cave;
    }
    
    for (int i = 0; i < 3; i++) {
        float angle = i * 2.09f;
        int x = centerX + (int)(cos(angle) * 50);
        int y = centerY + (int)(sin(angle) * 50);
        
        Building* hut = CreateHouse(x, y, 0);
        if (hut) {
            DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
            SetBuildingDoor(hut, doorPos);
            PlaceBuildingOnMap(map, hut);
            map->buildingCount++;
            map->buildings = (Building**)realloc(map->buildings, 
                               map->buildingCount * sizeof(Building*));
            map->buildings[map->buildingCount - 1] = hut;
        }
    }
    
    LOG("Cave placed with %d buildings\n", map->buildingCount);
}

void PlaceTower(LocalMap* map)
{
    if (!map) return;
    
    LOG("  Placing wizard tower...\n");
    
    int centerX = LOCAL_MAP_WIDTH / 2;
    int centerY = LOCAL_MAP_HEIGHT / 2;
    
    for (int y = centerY - 40; y <= centerY + 40; y++) {
        for (int x = centerX - 40; x <= centerX + 40; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                ClearTileKeepWater(map, x, y);
            }
        }
    }
    
    Building* tower = CreateTower(centerX - 8, centerY - 8, 2);
    if (tower) {
        DoorPosition doorPos = GetDoorPositionForBuilding(centerX - 8, centerY - 8, centerX, centerY);
        SetBuildingDoor(tower, doorPos);
        PlaceBuildingOnMap(map, tower);
        map->buildingCount++;
        map->buildings = (Building**)realloc(map->buildings, 
                           map->buildingCount * sizeof(Building*));
        map->buildings[map->buildingCount - 1] = tower;
    }
    
    for (int i = 0; i < 3; i++) {
        float angle = i * 2.09f;
        int x = centerX + (int)(cos(angle) * 30);
        int y = centerY + (int)(sin(angle) * 30);
        
        Building* house = CreateHouse(x, y, 1);
        if (house) {
            DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
            SetBuildingDoor(house, doorPos);
            PlaceBuildingOnMap(map, house);
            map->buildingCount++;
            map->buildings = (Building**)realloc(map->buildings, 
                               map->buildingCount * sizeof(Building*));
            map->buildings[map->buildingCount - 1] = house;
        }
    }
    
    LOG("  Tower placed with %d buildings\n", map->buildingCount);
}

void PlaceDungeon(LocalMap* map)
{
    if (!map) return;
    
    LOG("  Placing dungeon...\n");
    
    int centerX = LOCAL_MAP_WIDTH / 2;
    int centerY = LOCAL_MAP_HEIGHT / 2;
    
    for (int y = centerY - 50; y <= centerY + 50; y++) {
        for (int x = centerX - 50; x <= centerX + 50; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                char& t = map->tiles[y][x];
                if (rand() % 100 < 6 && TileWalkable(t) && !TileIsWater(t) && t != TILE_ROAD) t = TILE_BOULDER;
            }
        }
    }
    
    Building* entrance = CreateDungeonEntrance(centerX - 10, centerY - 5, 2);
    if (entrance) {
        DoorPosition doorPos = GetDoorPositionForBuilding(centerX - 10, centerY - 5, centerX, centerY);
        SetBuildingDoor(entrance, doorPos);
        PlaceBuildingOnMap(map, entrance);
        map->buildingCount++;
        map->buildings = (Building**)realloc(map->buildings, 
                           map->buildingCount * sizeof(Building*));
        map->buildings[map->buildingCount - 1] = entrance;
    }
    
    for (int i = 0; i < 4; i++) {
        float angle = i * 1.57f;
        int x = centerX + (int)(cos(angle) * 40);
        int y = centerY + (int)(sin(angle) * 40);
        
        Building* post = CreateGuardPost(x, y, 0);
        if (post) {
            DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
            SetBuildingDoor(post, doorPos);
            PlaceBuildingOnMap(map, post);
            map->buildingCount++;
            map->buildings = (Building**)realloc(map->buildings, 
                               map->buildingCount * sizeof(Building*));
            map->buildings[map->buildingCount - 1] = post;
        }
    }
    
    LOG("  Dungeon placed with %d buildings\n", map->buildingCount);
}

void PlaceRuins(LocalMap* map)
{
    if (!map) return;
    
    LOG("  Placing ruins...\n");
    
    int centerX = LOCAL_MAP_WIDTH / 2;
    int centerY = LOCAL_MAP_HEIGHT / 2;
    
    for (int y = centerY - 60; y <= centerY + 60; y++) {
        for (int x = centerX - 60; x <= centerX + 60; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                char& t = map->tiles[y][x];
                int r = rand() % 100;
                if (r < 6 && t == TILE_GRASS) t = TILE_TREE;                 // overgrown ruins
                else if (r < 30 && t == TILE_GRASS) t = TILE_TALLGRASS;
            }
        }
    }
    
    int numRuins = 3 + (rand() % 4);
    for (int i = 0; i < numRuins; i++) {
        float angle = i * 1.57f;
        int x = centerX + (int)(cos(angle) * (20 + rand() % 30));
        int y = centerY + (int)(sin(angle) * (20 + rand() % 30));
        
        Building* ruin = CreateRuin(x, y, 1);
        if (ruin) {
            DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
            SetBuildingDoor(ruin, doorPos);
            PlaceBuildingOnMap(map, ruin);
            map->buildingCount++;
            map->buildings = (Building**)realloc(map->buildings, 
                               map->buildingCount * sizeof(Building*));
            map->buildings[map->buildingCount - 1] = ruin;
        }
    }
    
    LOG("  Ruins placed with %d buildings\n", map->buildingCount);
}

void PlaceFort(LocalMap* map)
{
    if (!map) return;
    
    LOG("  Placing fort...\n");
    
    int centerX = LOCAL_MAP_WIDTH / 2;
    int centerY = LOCAL_MAP_HEIGHT / 2;
    
    for (int y = centerY - 50; y <= centerY + 50; y++) {
        for (int x = centerX - 50; x <= centerX + 50; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                ClearTileKeepWater(map, x, y);
            }
        }
    }
    
    int numBarracks = 2 + (rand() % 2);
    for (int i = 0; i < numBarracks; i++) {
        int x = centerX - 30 + i * 40;
        int y = centerY - 10;
        
        Building* barracks = CreateGuardPost(x, y, 2);
        if (barracks) {
            DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
            SetBuildingDoor(barracks, doorPos);
            PlaceBuildingOnMap(map, barracks);
            map->buildingCount++;
            map->buildings = (Building**)realloc(map->buildings, 
                               map->buildingCount * sizeof(Building*));
            map->buildings[map->buildingCount - 1] = barracks;
        }
    }
    
    int corners[4][2] = {
        {centerX - 45, centerY - 30},
        {centerX + 35, centerY - 30},
        {centerX - 45, centerY + 20},
        {centerX + 35, centerY + 20}
    };
    
    for (int i = 0; i < 4; i++) {
        Building* post = CreateGuardPost(corners[i][0], corners[i][1], 1);
        if (post) {
            DoorPosition doorPos = GetDoorPositionForBuilding(corners[i][0], corners[i][1], centerX, centerY);
            SetBuildingDoor(post, doorPos);
            PlaceBuildingOnMap(map, post);
            map->buildingCount++;
            map->buildings = (Building**)realloc(map->buildings, 
                               map->buildingCount * sizeof(Building*));
            map->buildings[map->buildingCount - 1] = post;
        }
    }
    
    Building* smith = CreateBlacksmith(centerX + 30, centerY + 20, 1);
    if (smith) {
        DoorPosition doorPos = GetDoorPositionForBuilding(centerX + 30, centerY + 20, centerX, centerY);
        SetBuildingDoor(smith, doorPos);
        PlaceBuildingOnMap(map, smith);
        map->buildingCount++;
        map->buildings = (Building**)realloc(map->buildings, 
                           map->buildingCount * sizeof(Building*));
        map->buildings[map->buildingCount - 1] = smith;
    }
    
    LOG("  Fort placed with %d buildings\n", map->buildingCount);
}

void PlaceTemple(LocalMap* map)
{
    if (!map) return;
    
    LOG("  Placing temple...\n");
    
    int centerX = LOCAL_MAP_WIDTH / 2;
    int centerY = LOCAL_MAP_HEIGHT / 2;
    
    for (int y = centerY - 40; y <= centerY + 40; y++) {
        for (int x = centerX - 40; x <= centerX + 40; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                ClearTileKeepWater(map, x, y);
            }
        }
    }
    
    Building* temple = CreateTemple(centerX - 12, centerY - 10, 2);
    if (temple) {
        DoorPosition doorPos = GetDoorPositionForBuilding(centerX - 12, centerY - 10, centerX, centerY);
        SetBuildingDoor(temple, doorPos);
        PlaceBuildingOnMap(map, temple);
        map->buildingCount++;
        map->buildings = (Building**)realloc(map->buildings, 
                           map->buildingCount * sizeof(Building*));
        map->buildings[map->buildingCount - 1] = temple;
    }
    
    for (int i = 0; i < 3; i++) {
        float angle = i * 2.09f;
        int x = centerX + (int)(cos(angle) * 35);
        int y = centerY + (int)(sin(angle) * 35);
        
        Building* house = CreateHouse(x, y, 1);
        if (house) {
            DoorPosition doorPos = GetDoorPositionForBuilding(x, y, centerX, centerY);
            SetBuildingDoor(house, doorPos);
            PlaceBuildingOnMap(map, house);
            map->buildingCount++;
            map->buildings = (Building**)realloc(map->buildings, 
                               map->buildingCount * sizeof(Building*));
            map->buildings[map->buildingCount - 1] = house;
        }
    }
    
    // Sacred garden
    int gardenX = centerX + 40;
    int gardenY = centerY - 15;
    for (int y = gardenY - 5; y <= gardenY + 5; y++) {
        for (int x = gardenX - 5; x <= gardenX + 5; x++) {
            if (x >= 0 && x < map->width && y >= 0 && y < map->height) {
                if (rand() % 3 == 0 && TileWalkable(map->tiles[y][x]) && !TileIsWater(map->tiles[y][x])) map->tiles[y][x] = TILE_FLOWERS;
            }
        }
    }
    
    LOG("  Temple placed with %d buildings\n", map->buildingCount);
}

void PlaceStructuresForLocation(LocalMap* map, LocationType locationType)
{
    switch (locationType) {
        case LOCATION_VILLAGE:
            PlaceVillage(map);
            break;
        case LOCATION_CASTLE:
            PlaceCastle(map);
            break;
        case LOCATION_CAVE:
            PlaceCave(map);
            break;
        case LOCATION_WIZARD_TOWER:
            PlaceTower(map);
            break;
        case LOCATION_DUNGEON:
            PlaceDungeon(map);
            break;
        case LOCATION_RUINS:
            PlaceRuins(map);
            break;
        case LOCATION_FORT:
            PlaceFort(map);
            break;
        case LOCATION_TEMPLE:
            PlaceTemple(map);
            break;
        default:
            break;
    }
}

// ============================================================================
// NPC MANAGEMENT
// ============================================================================

void SpawnNPCsForMap(LocalMap* map, int worldX, int worldY)
{
    LOG("  SpawnNPCsForMap starting...\n");
    
    if (!map) {
        LOG("  ERROR: map is NULL\n");
        return;
    }
    
    LOG("  map has %d spawn points\n", map->spawnPointCount);
    
    if (!map->spawnPoints) {
        LOG("  ERROR: map->spawnPoints is NULL!\n");
        return;
    }
    
    if (map->spawnPointCount == 0) {
        LOG("  No spawn points for map (%d,%d)\n", worldX, worldY);
        return;
    }
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        if (!map->spawnPoints[i]) {
            LOG("  ERROR: spawn point %d is NULL!\n", i);
        } else {
            LOG("  Spawn point %d: at (%d,%d), type=%d, maxCount=%d\n", 
                i, map->spawnPoints[i]->x, map->spawnPoints[i]->y,
                map->spawnPoints[i]->type, map->spawnPoints[i]->maxCount);
        }
    }
    
    bool npcsExist = false;
    for (int i = 0; i < npcCount; i++) {
        if (npcs[i] && npcs[i]->worldX == worldX && npcs[i]->worldY == worldY) {
            npcsExist = true;
            break;
        }
    }
    
    if (npcsExist) {
        LOG("  NPCs already exist for map (%d,%d)\n", worldX, worldY);
        return;
    }
    
    LOG("  Spawning NPCs...\n");
    
    for (int i = 0; i < map->spawnPointCount; i++) {
        SpawnPoint* spawn = map->spawnPoints[i];
        if (!spawn) continue;
        
        LOG("    Creating %d NPCs from spawn %d\n", spawn->maxCount, i);
        
        for (int j = 0; j < spawn->maxCount; j++) {
            NPC* npc = CreateNPCFromSpawn(spawn, worldX, worldY);
            if (npc) {
                NPC** newArray = (NPC**)realloc(npcs, (npcCount + 1) * sizeof(NPC*));
                if (!newArray) {
                    LOG("    ERROR: Failed to realloc\n");
                    return;
                }
                npcs = newArray;
                npcs[npcCount] = npc;
                npcCount++;
                spawn->currentCount++;
                LOG("      Spawned NPC %d\n", npcCount);
            } else {
                LOG("      Failed to create NPC\n");
            }
        }
    }
    
    LOG("  Spawned NPCs, total now: %d\n", npcCount);
}

// ============================================================================
// CORE LOCAL MAP FUNCTIONS
// ============================================================================

void DrawLocalMap()
{
    if (!worldMap) {
        LOG("Error: worldMap is NULL in DrawLocalMap\n");
        return;
    }
    
    if (player.worldPos.y < 0 || player.worldPos.y >= currentMapHeight ||
        player.worldPos.x < 0 || player.worldPos.x >= currentMapWidth) {
        LOG("Error: Invalid world position (%d,%d) in DrawLocalMap - exiting\n", 
               player.worldPos.x, player.worldPos.y);
        ExitLocalMap();
        return;
    }
    
    WorldTile* currentTile = &worldMap[player.worldPos.y][player.worldPos.x];
    if (currentTile->localMap == NULL) {
        LOG("Error: localMap is NULL in DrawLocalMap - exiting\n");
        ExitLocalMap();
        return;
    }
    
    LocalMap* local = currentTile->localMap;
    
    if (player.localPos.x < 0 || player.localPos.x >= LOCAL_MAP_WIDTH ||
        player.localPos.y < 0 || player.localPos.y >= LOCAL_MAP_HEIGHT) {
        LOG("CRASH PREVENTION: Invalid local position (%d,%d) - resetting\n", 
               player.localPos.x, player.localPos.y);
        player.localPos.x = LOCAL_MAP_WIDTH / 2;
        player.localPos.y = LOCAL_MAP_HEIGHT / 2;
    }
    
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    float visibleWidth = screenWidth / gameCamera.camera.zoom;
    float visibleHeight = screenHeight / gameCamera.camera.zoom;
    
    Vector2 cameraWorldTopLeft = {
        gameCamera.camera.target.x - visibleWidth / 2.0f,
        gameCamera.camera.target.y - visibleHeight / 2.0f
    };
    
    DrawRectangle(cameraWorldTopLeft.x, cameraWorldTopLeft.y, 
                  visibleWidth, visibleHeight, BLACK);
    
    int startX = (int)(cameraWorldTopLeft.x / TILE_SIZE) - 1;
    int startY = (int)(cameraWorldTopLeft.y / TILE_SIZE) - 1;
    int endX = (int)((cameraWorldTopLeft.x + visibleWidth) / TILE_SIZE) + 2;
    int endY = (int)((cameraWorldTopLeft.y + visibleHeight) / TILE_SIZE) + 2;
    
    if (startX < 0) startX = 0;
    if (startY < 0) startY = 0;
    if (endX > local->width) endX = local->width;
    if (endY > local->height) endY = local->height;
    
    const int wx = player.worldPos.x, wy = player.worldPos.y;
    for (int y = startY; y < endY; y++)
    {
        for (int x = startX; x < endX; x++)
        {
            char tile = local->tiles[y][x];
            int biome = local->biome ? local->biome[y * local->width + x] : BIOME_GRASSLAND;
            unsigned int h = HashCell(x + wx * LOCAL_MAP_WIDTH, y + wy * LOCAL_MAP_HEIGHT, 1337u);   // stable per cell, continuous across tiles
            DrawGlyphCell(TileGlyph(tile, h), x, y, TileColor(tile, biome, h), TileBackground(tile, biome, h));
        }
    }
}

void GenerateLocalMap(int worldX, int worldY)
{
    if (!worldMap) return;
    WorldTile* tile = &worldMap[worldY][worldX];
    if (!tile->hasLocalMap) return;
    
    LOG("\n=== GENERATING LOCAL MAP FOR (%d,%d) ===\n", worldX, worldY);
    
    tile->discovered = true;
    
    LocalMap* local = (LocalMap*)malloc(sizeof(LocalMap));
    if (!local) {
        LOG("ERROR: Failed to allocate LocalMap structure\n");
        return;
    }
    
    local->width = LOCAL_MAP_WIDTH;
    local->height = LOCAL_MAP_HEIGHT;
    local->buildings = NULL;
    local->buildingCount = 0;
    local->biome = (unsigned char*)calloc((size_t)local->width * local->height, 1);
    local->spawnPoints = NULL;
    local->spawnPointCount = 0;
    
    local->tiles = (char**)calloc(local->height, sizeof(char*));
    if (!local->tiles) {
        LOG("ERROR: Failed to allocate tile rows\n");
        free(local);
        return;
    }
    
    for (int y = 0; y < local->height; y++) {
        local->tiles[y] = (char*)calloc(local->width + 1, sizeof(char));
        if (!local->tiles[y]) {
            LOG("ERROR: Failed to allocate tile row %d\n", y);
            for (int i = 0; i < y; i++) free(local->tiles[i]);
            free(local->tiles);
            free(local);
            return;
        }
    }
    
    LOG("Map size: %dx%d\n", local->width, local->height);
    
    // Everything below is deterministic per (world seed, tile): the same tile always regenerates identically
    unsigned int localSeed = HashCell(worldX, worldY, worldSeed);
    srand(localSeed);

    GenerateLocalTerrain(local, worldX, worldY);
    
    if (tile->locationType != LOCATION_NONE) {
        PlaceStructuresForLocation(local, tile->locationType);
    }
    
    GenerateSpawnPoints(local, tile->locationType);
    
    if (local->spawnPointCount > 0 && !local->spawnPoints) {
        LOG("ERROR: Spawn points count > 0 but array is NULL!\n");
        local->spawnPointCount = 0;
    }

    tile->localMap = local;
    srand((unsigned int)time(NULL) ^ (unsigned int)clock() ^ (localSeed * 2654435761u));   // back to unpredictable randomness
    LOG("=== LOCAL MAP GENERATION COMPLETE ===\n\n");
}

// Finds the nearest walkable, unoccupied cell to (x,y) that belongs to a reasonably large connected area.
static bool FindWalkableSpot(LocalMap* map, int worldX, int worldY, int* ox, int* oy)
{
    const int W = map->width, H = map->height;
    std::vector<char> rejected((size_t)W * H, 0);
    std::vector<int> queue, seen((size_t)W * H, 0);
    int stamp = 0;
    for (int r = 0; r < 128; r++) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (r > 0 && abs(dx) != r && abs(dy) != r) continue;       // ring only
                int x = *ox + dx, y = *oy + dy;
                if (x < 1 || y < 1 || x >= W - 1 || y >= H - 1) continue;
                if (rejected[y * W + x] || !IsTileWalkable(map->tiles[y][x])) continue;
                if (GetNPCAt(worldX, worldY, x, y)) continue;
                // is this part of a big enough walkable area? (flood fill, capped)
                stamp++;
                queue.clear(); queue.push_back(y * W + x); seen[y * W + x] = stamp;
                for (size_t qi = 0; qi < queue.size() && queue.size() < 400; qi++) {
                    int c = queue[qi], cx = c % W, cy = c / W;
                    static const int DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };
                    for (int d = 0; d < 4; d++) {
                        int nx = cx + DX[d], ny = cy + DY[d];
                        if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                        int ni = ny * W + nx;
                        if (seen[ni] == stamp || !IsTileWalkable(map->tiles[ny][nx])) continue;
                        seen[ni] = stamp; queue.push_back(ni);
                    }
                }
                if (queue.size() >= 400) { *ox = x; *oy = y; return true; }
                for (int c : queue) rejected[c] = 1;
            }
        }
    }
    return false;
}

// Walk off the edge of the current local map into the neighbouring world tile - the world is seamless.
bool TravelToAdjacentTile(int dx, int dy)
{
    int nwx = player.worldPos.x + dx, nwy = player.worldPos.y + dy;
    if (nwx < 0 || nwy < 0 || nwx >= currentMapWidth || nwy >= currentMapHeight) return false;
    WorldTile* dest = &worldMap[nwy][nwx];
    if (!dest->hasLocalMap) return false;
    if (!dest->localMap) GenerateLocalMap(nwx, nwy);
    if (!dest->localMap) return false;

    int nx = player.localPos.x, ny = player.localPos.y;
    if (dx > 0) nx = 0; else if (dx < 0) nx = dest->localMap->width - 1;
    if (dy > 0) ny = 0; else if (dy < 0) ny = dest->localMap->height - 1;

    if (!IsTileWalkable(dest->localMap->tiles[ny][nx])) return false;       // a tree or deep water in the way, like anywhere else
    if (GetNPCAt(nwx, nwy, nx, ny)) return false;

    RemoveNPCsFromMap(player.worldPos.x, player.worldPos.y);
    player.worldPos.x = nwx;
    player.worldPos.y = nwy;
    player.localPos.x = nx;
    player.localPos.y = ny;
    RevealAround(nwx, nwy, 1);
    SpawnNPCsForMap(dest->localMap, nwx, nwy);
    SafeInitCamera(false);          // snap the camera, don't glide across a whole map
    return true;
}

void EnterLocalMap(int worldX, int worldY)
{
    LOG("\n=== ENTER LOCAL MAP DEBUG ===\n");
    LOG("Step 0: Starting EnterLocalMap(%d,%d)\n", worldX, worldY);
    
    if (!worldMap) {
        LOG("ERROR: worldMap is NULL in EnterLocalMap\n");
        return;
    }
    LOG("Step 1: worldMap OK at %p\n", (void*)worldMap);
    
    LOG("Current map size: %dx%d\n", currentMapWidth, currentMapHeight);
    
    if (worldX < 0 || worldX >= currentMapWidth || 
        worldY < 0 || worldY >= currentMapHeight) {
        LOG("ERROR: Invalid world coordinates (%d,%d)\n", worldX, worldY);
        return;
    }
    LOG("Step 2: Coordinates valid\n");
    
    LOG("Accessing worldMap[%d][%d]\n", worldY, worldX);
    WorldTile* tile = &worldMap[worldY][worldX];
    LOG("Step 3: Got tile at %p\n", (void*)tile);
    
    LOG("Tile data: type='%c', hasLocalMap=%d, locationType=%d\n", 
        tile->worldTile, tile->hasLocalMap, tile->locationType);
    
    if (!tile->hasLocalMap) {
        LOG("Warning: Tile has no local map\n");
        return;
    }
    
    LOG("Step 4: hasLocalMap OK\n");
    
    if (!tile->localMap) {
        LOG("Generating local map...\n");
        GenerateLocalMap(worldX, worldY);
        LOG("Local map generation complete\n");
    }
    
    if (!tile->localMap) {
        LOG("ERROR: Failed to generate local map\n");
        return;
    }
    LOG("Step 5: localMap exists at %p\n", (void*)tile->localMap);
    
    LOG("Local map stats: buildings=%d, spawnPoints=%d\n", 
        tile->localMap->buildingCount, tile->localMap->spawnPointCount);
    
    LOG("Step 6: Spawning NPCs...\n");
    SpawnNPCsForMap(tile->localMap, worldX, worldY);
    LOG("Step 7: NPC spawning complete\n");
    
    LOG("Setting player position...\n");
    LOG("hasExitPoint = %d\n", tile->hasExitPoint);
    
    if (tile->hasExitPoint) {
        LOG("Using saved exit point (%d,%d)\n", tile->lastExitX, tile->lastExitY);
        player.localPos.x = tile->lastExitX;
        player.localPos.y = tile->lastExitY;
    } else {
        LOG("First time entering, using center (%d,%d)\n", 
            LOCAL_MAP_WIDTH/2, LOCAL_MAP_HEIGHT/2);
        player.localPos.x = LOCAL_MAP_WIDTH / 2;
        player.localPos.y = LOCAL_MAP_HEIGHT / 2;
    }
    
    LOG("Position set to (%d,%d)\n", player.localPos.x, player.localPos.y);
    
    if (player.localPos.x < 1) player.localPos.x = 1;
    if (player.localPos.y < 1) player.localPos.y = 1;
    if (player.localPos.x >= LOCAL_MAP_WIDTH - 1) player.localPos.x = LOCAL_MAP_WIDTH - 2;
    if (player.localPos.y >= LOCAL_MAP_HEIGHT - 1) player.localPos.y = LOCAL_MAP_HEIGHT - 2;
    
    {   // never start inside a wall, a fountain or a sealed pocket
        int sx = player.localPos.x, sy = player.localPos.y;
        if (FindWalkableSpot(tile->localMap, worldX, worldY, &sx, &sy)) { player.localPos.x = sx; player.localPos.y = sy; }
    }
    LOG("Final position after clamping: (%d,%d)\n", player.localPos.x, player.localPos.y);
    
    player.isInLocalMap = true;
    LOG("isInLocalMap set to true\n");
    
    LOG("Calling SafeInitCamera(false)...\n");
    SafeInitCamera(false);
    LOG("Camera initialized\n");
    
    LOG("=== ENTER LOCAL MAP COMPLETE ===\n\n");
}

void ExitLocalMap()
{
    if (!player.isInLocalMap) return;
    
    WorldTile* tile = &worldMap[player.worldPos.y][player.worldPos.x];
    
    tile->lastExitX = player.localPos.x;
    tile->lastExitY = player.localPos.y;
    tile->hasExitPoint = true;
    
    RemoveNPCsFromMap(player.worldPos.x, player.worldPos.y);
    
    player.isInLocalMap = false;
    SafeInitCamera(false);
}

void RemoveNPCsFromMap(int worldX, int worldY)
{
    if (!npcs || npcCount == 0) return;
    
    int removed = 0;
    for (int i = 0; i < npcCount; i++) {
        if (npcs[i] && npcs[i]->worldX == worldX && npcs[i]->worldY == worldY) {
            free(npcs[i]);
            npcs[i] = NULL;
            removed++;
        }
    }
    
    if (worldX >= 0 && worldY >= 0 && worldX < currentMapWidth && worldY < currentMapHeight && worldMap[worldY][worldX].localMap) {
        LocalMap* lm = worldMap[worldY][worldX].localMap;
        for (int i = 0; i < lm->spawnPointCount; i++) if (lm->spawnPoints[i]) lm->spawnPoints[i]->currentCount = 0;
    }

    if (removed == 0) return;
    
    int newCount = 0;
    for (int i = 0; i < npcCount; i++) {
        if (npcs[i] != NULL) {
            npcs[newCount++] = npcs[i];
        }
    }
    npcCount = newCount;
}