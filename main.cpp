#include "project.h"
#include "Game/GameState.h"
#include "World/WorldMap.h"
#include "Entities/Player.h"
#include "Systems/SaveLoad.h"
#include "Systems/CrashHandler.h"
#include "Systems/Logger.h"
#include "Render/Tileset.h"
#include "Systems/DataLoader.h"

int main()
{
    LOG("Program started (v%s)\n", GAME_VERSION);

    SetupCrashHandler();

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "BoneBound");
    LOG("Window initialized\n");
    LoadGameData(gameData);
    // FIX: raylib quits on ESC by default, which closed the game when cancelling a name edit or
    // leaving a conversation. ESC now belongs to the game (pause menu); quit from the menus instead.
    SetExitKey(KEY_NULL);

    SetTargetFPS(60);
    InitAudioDevice();
    srand((unsigned int)time(NULL));

    TilesetInit("resources/tileset/DB_curses_12x12.png");

    memset(&player, 0, sizeof(PlayerData));
    player.localPos.x = LOCAL_MAP_WIDTH / 2;
    player.localPos.y = LOCAL_MAP_HEIGHT / 2;
    player.isInLocalMap = false;

    currentState = STATE_TITLE;
    shouldQuit = false;

    while (!WindowShouldClose() && !shouldQuit)
    {
        static int frameCounter = 0;
        if (++frameCounter % 1800 == 0) {
            CheckMemoryUsage();
        }

        GameUpdate();
        GameDraw();
    }

    CleanupAllMaps();          // also joins the world generator thread if it is still running
    TilesetShutdown();
    CloseAudioDevice();
    CloseWindow();

    CloseLogger();
    return 0;
}
