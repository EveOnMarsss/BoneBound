#include "HUD.h"
#include "../World/LocationNames.h"
#include "../World/Tiles.h"
#include "../World/LocalMap.h"

const char* LocationTypeName(LocationType t)
{
    switch (t) {
        case LOCATION_VILLAGE:      return "Village";
        case LOCATION_CASTLE:       return "Castle";
        case LOCATION_CAVE:         return "Cave";
        case LOCATION_WIZARD_TOWER: return "Wizard's Tower";
        case LOCATION_DUNGEON:      return "Dungeon";
        case LOCATION_RUINS:        return "Ruins";
        case LOCATION_FORT:         return "Fort";
        case LOCATION_TEMPLE:       return "Temple";
        default:                    return NULL;
    }
}

const char* TemperatureWord(float t)
{
    if (t < 0.15f) return "freezing";
    if (t < 0.30f) return "cold";
    if (t < 0.50f) return "cool";
    if (t < 0.72f) return "mild";
    if (t < 0.88f) return "warm";
    return "hot";
}

static void DrawCentered(const char* text, float y, int size, Color c)
{
    Vector2 sz = MeasureTextEx(GetFontDefault(), text, (float)size, 1);
    DrawTextEx(GetFontDefault(), text, (Vector2){ GetScreenWidth() / 2.0f - sz.x / 2.0f, y }, (float)size, 1, c);
}

void DrawHUD()
{
    int screenHeight = GetScreenHeight();
    int screenWidth = GetScreenWidth();
    (void)screenWidth;

    // ---- character panel (top-left) ----
    DrawRectangle(6, 6, 300, 54, (Color){ 0, 0, 0, 150 });
    DrawText(TextFormat("%s the %s", player.name, player.isCat ? "Cat" : "Dog"), 14, 10, 20, YELLOW);
    DrawText(TextFormat("Lv %d   HP %d/%d   Gold %d", player.level, player.currentHealth, player.maxHealth, player.gold), 14, 34, 16, LIGHTGRAY);

    WorldTile* tile = &worldMap[player.worldPos.y][player.worldPos.x];
    const char* typeStr = LocationTypeName(tile->locationType);

    if (player.isInLocalMap) {
        DrawCentered(tile->locationName, 12, 28, GOLD);
        char sub[128];
        snprintf(sub, sizeof(sub), "%s  -  %s, %s", typeStr ? typeStr : "Wilderness", BiomeName(tile->biome), TemperatureWord(tile->temperature));
        DrawCentered(sub, 46, 18, LIGHTGRAY);

        // what is underfoot
        LocalMap* lm = tile->localMap;
        if (lm && player.localPos.x >= 0 && player.localPos.y >= 0 && player.localPos.x < lm->width && player.localPos.y < lm->height) {
            DrawText(TextFormat("Standing on: %s", TileName(lm->tiles[player.localPos.y][player.localPos.x])), 10, screenHeight - 80, 16, LIGHTGRAY);
        }
        DrawText(TextFormat("Tile (%d,%d)  Cell (%d,%d)", player.worldPos.x, player.worldPos.y, player.localPos.x, player.localPos.y), 10, screenHeight - 58, 16, LIGHTGRAY);
        DrawText("Walk off any edge to continue into the next area  |  BACKSPACE: world map  |  E: Talk", 10, screenHeight - 32, 16, GRAY);
    } else {
        if (tile->discovered) {
            DrawText(TextFormat("%s", tile->locationName), 10, screenHeight - 80, 22, GOLD);
            char sub[128];
            snprintf(sub, sizeof(sub), "%s%s%s%s", typeStr ? typeStr : BiomeName(tile->biome),
                     typeStr ? ", " : "", typeStr ? BiomeName(tile->biome) : "",
                     (tile->riverMask && !tile->isLake) ? "  (river)" : "");
            DrawText(sub, 10, screenHeight - 54, 16, LIGHTGRAY);
        } else {
            DrawText("Unexplored", 10, screenHeight - 80, 22, GRAY);
        }
        DrawText(TextFormat("World (%d,%d)   seed %u", player.worldPos.x, player.worldPos.y, worldSeed), 10, screenHeight - 32, 16, GRAY);
        DrawCentered("ENTER: explore this area  |  Wheel: zoom  |  ESC: menu", (float)screenHeight - 30, 16, GRAY);
    }

    DrawText("WASD/Arrows: move   R: reset camera   F5: save   F9: load   F11: fullscreen", 10, screenHeight - 105 + (player.isInLocalMap ? 0 : 25), 14, DARKGRAY);
}
