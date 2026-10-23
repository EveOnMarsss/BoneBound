#include "GameState.h"
#include "../World/WorldMap.h"
#include "../World/LocalMap.h"
#include "../Entities/Player.h"
#include "../Entities/NPC.h"
#include "../Entities/SpawnPoint.h"
#include "../UI/Menus.h"
#include "../UI/HUD.h"
#include "../UI/Conversation.h"
#include "../Systems/Input.h"
#include "../Systems/SaveLoad.h"
#include "Camera.h"




void GameStartup()
{
    player.isInLocalMap = false;
    player.localPos.x = LOCAL_MAP_WIDTH / 2;
    player.localPos.y = LOCAL_MAP_HEIGHT / 2;
}

void GameUpdate()
{
    if (shouldQuit) return;

    // A world must exist to be in these states
    if ((currentState == STATE_PLAYING || currentState == STATE_PAUSE || currentState == STATE_CHOOSE_START) && !worldMap) {
        currentState = STATE_TITLE;
        selectedOption = 0;
        return;
    }

    // Fullscreen: F11 (the old F key toggled fullscreen while you were typing a name containing 'f')
    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen(SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    if (isInConversation) {
        UpdateConversation();
        return;
    }

    switch (currentState) {
        case STATE_TITLE:              TitleUpdate(); break;
        case STATE_MAPSIZE:            MapSizeUpdate(); break;
        case STATE_SAVE_MENU:          SaveMenuUpdate(); break;
        case STATE_LOAD_MENU:          LoadMenuUpdate(); break;
        case STATE_CHARACTER_CREATION: CharacterCreationUpdate(); break;
        case STATE_GENERATING:         GeneratingUpdate(); break;
        case STATE_CHOOSE_START:       StartPickUpdate(); break;
        case STATE_PAUSE:              PauseUpdate(); break;

        case STATE_PLAYING:
            if (IsKeyPressed(KEY_ESCAPE)) {
                currentState = STATE_PAUSE;
                selectedOption = 0;
                break;
            }
            HandleInput();
            if (currentState != STATE_PLAYING) break;
            UpdateCamera();
            UpdateNPCs();
            UpdateSpawnPoints();

            if (IsKeyPressed(KEY_F5)) { menuReturnState = STATE_PLAYING; currentState = STATE_SAVE_MENU; saveSlotSelected = 0; }
            if (IsKeyPressed(KEY_F9)) { menuReturnState = STATE_PLAYING; currentState = STATE_LOAD_MENU; saveSlotSelected = 0; }
            break;
        default: break;
    }
}

static void DrawPlayingScene()
{
    BeginMode2D(gameCamera.camera);
    if (player.isInLocalMap) {
        DrawLocalMap();
        DrawNPCs();
        DrawLocalPlayer();
    } else {
        DrawWorldMap();
        DrawWorldPlayer();
    }
    EndMode2D();
    DrawHUD();
}

void GameDraw()
{
    BeginDrawing();
    ClearBackground(BLACK);

    switch (currentState) {
        case STATE_TITLE:              TitleDraw(); break;
        case STATE_MAPSIZE:            MapSizeDraw(); break;
        case STATE_SAVE_MENU:          SaveMenuDraw(); break;
        case STATE_LOAD_MENU:          LoadMenuDraw(); break;
        case STATE_CHARACTER_CREATION: CharacterCreationDraw(); break;
        case STATE_GENERATING:         GeneratingDraw(); break;
        case STATE_CHOOSE_START:       StartPickDraw(); break;
        case STATE_PLAYING:
            DrawPlayingScene();
            if (isInConversation) DrawConversation();
            break;
        case STATE_PAUSE:
            DrawPlayingScene();
            PauseDraw();
            break;
        default: break;
    }

    EndDrawing();
}
