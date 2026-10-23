#include "Player.h"
#include "NPC.h"
#include "../World/LocalMap.h"
#include "../World/WorldMap.h"
#include "../Game/Camera.h"
#include "../Systems/Input.h"
#include "../Render/Tileset.h"
#include <stdio.h>

void UpdatePlayer()
{
    if (currentState != STATE_PLAYING) return;
    if (!worldMap) return;

    // Validate world position (fall back to the middle of the map, never to a hardcoded tile)
    if (player.worldPos.x < 0 || player.worldPos.x >= currentMapWidth ||
        player.worldPos.y < 0 || player.worldPos.y >= currentMapHeight) {
        LOG("ERROR: invalid world position (%d,%d) - resetting\n", player.worldPos.x, player.worldPos.y);
        player.worldPos.x = currentMapWidth / 2;
        player.worldPos.y = currentMapHeight / 2;
    }
    if (player.localPos.x < 0 || player.localPos.x >= LOCAL_MAP_WIDTH ||
        player.localPos.y < 0 || player.localPos.y >= LOCAL_MAP_HEIGHT) {
        player.localPos.x = LOCAL_MAP_WIDTH / 2;
        player.localPos.y = LOCAL_MAP_HEIGHT / 2;
    }

    int dx = 0, dy = 0;

    if (player.isInLocalMap) {
        // ============= LOCAL MAP MOVEMENT =============
        WorldTile* tile = &worldMap[player.worldPos.y][player.worldPos.x];
        if (!tile->localMap) {
            LOG("ERROR: localMap is NULL in UpdatePlayer - exiting local map\n");
            ExitLocalMap();
            return;
        }
        LocalMap* map = tile->localMap;

        if (ReadMoveStep(&dx, &dy, 0.20f, 0.055f)) {
            int nx = player.localPos.x + dx, ny = player.localPos.y + dy;
            if (nx < 0 || ny < 0 || nx >= map->width || ny >= map->height) {
                // walked off the edge: continue into the neighbouring world tile
                TravelToAdjacentTile(dx, dy);
                return;                         // 'map' may no longer be the current map
            }
            if (IsTileWalkable(map->tiles[ny][nx]) && !GetNPCAt(player.worldPos.x, player.worldPos.y, nx, ny)) {
                player.localPos.x = nx;
                player.localPos.y = ny;
            }
        }

        if (IsKeyPressed(KEY_BACKSPACE)) { ExitLocalMap(); return; }
        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE)) InteractWithNPC();
    } else {
        // ============= WORLD MAP MOVEMENT =============
        if (ReadMoveStep(&dx, &dy, 0.25f, 0.09f)) {
            int nx = player.worldPos.x + dx, ny = player.worldPos.y + dy;
            if (WorldTileEnterable(nx, ny)) {          // can't walk across open ocean or lakes
                player.worldPos.x = nx;
                player.worldPos.y = ny;
                RevealAround(nx, ny, 2);               // exploring uncovers the surrounding map
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            WorldTile* tile = &worldMap[player.worldPos.y][player.worldPos.x];
            if (tile->hasLocalMap) {
                EnterLocalMap(player.worldPos.x, player.worldPos.y);
                return;
            }
        }
        // (BACKSPACE no longer dumps you to the title screen and loses the game - use ESC for the pause menu)
    }
    // the camera is updated once per frame in GameUpdate
}

// ===== PLAYER DRAWING =====
// '@' is the classic roguelike hero glyph, and (unlike the old 'C'/'D') can't be mistaken for Castle/Dungeon.
static Color PlayerColor() { return player.isCat ? (Color){ 255, 190, 70, 255 } : (Color){ 255, 235, 130, 255 }; }

void DrawWorldPlayer()
{
    if (player.isInLocalMap) return;
    if (player.worldPos.x < 0 || player.worldPos.x >= currentMapWidth ||
        player.worldPos.y < 0 || player.worldPos.y >= currentMapHeight) return;
    // blinking so the hero is easy to find on a big map
    Color c = PlayerColor();
    if (((int)(GetTime() * 2.5)) % 2 == 1) c = WHITE;
    DrawGlyphCell('@', player.worldPos.x, player.worldPos.y, c, (Color){ 0, 0, 0, 255 });
}

void DrawLocalPlayer()
{
    if (!player.isInLocalMap) return;
    if (player.localPos.x < 0 || player.localPos.x >= LOCAL_MAP_WIDTH ||
        player.localPos.y < 0 || player.localPos.y >= LOCAL_MAP_HEIGHT) return;
    DrawGlyphCell('@', player.localPos.x, player.localPos.y, PlayerColor(), (Color){ 0, 0, 0, 255 });
}
