#include "Menus.h"
#include "../World/WorldMap.h"
#include "../Systems/SaveLoad.h"
#include "../World/WorldGen.h"
#include "../Systems/Input.h"
#include "../Game/Camera.h"
#include "HUD.h"
#include "../World/LocationNames.h"
#include <time.h>

// Where the Save / Load menus return to (title screen, pause menu or straight back into the game).
// FIX: they used to return to STATE_PLAYING whenever a world existed, even when opened from the title.
GameState menuReturnState = STATE_TITLE;
static int chosenMapSize = 2;          // FIX: the chosen size used to be read from selectedOption, which had been reused (always MEDIUM)
static char menuStatus[64] = "";
static double menuStatusUntil = 0.0;

// Character creation variables
char playerName[32] = "Hero";
int nameCursorPos = 4; // Length of "Hero"
bool isEditingName = false;
int raceSelected = 1; // 0 = Dog, 1 = Cat (default Cat)

void TitleUpdate()
{
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        selectedOption = (selectedOption + 1) % 4;
    
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        selectedOption = (selectedOption - 1 + 4) % 4;
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (selectedOption == 0) {
            currentState = STATE_MAPSIZE;
            selectedOption = chosenMapSize;
        } else if (selectedOption == 1) {
            if (SaveFileExists(0) || SaveFileExists(1) || SaveFileExists(2)) {
                menuReturnState = STATE_TITLE;
                saveSlotSelected = 0;
                currentState = STATE_LOAD_MENU;
            }
        } else if (selectedOption == 2) {
            ToggleFullscreen(SCREEN_WIDTH, SCREEN_HEIGHT);
        } else if (selectedOption == 3) {
            shouldQuit = true;
        }
    }
}

void TitleDraw()
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Version in bottom left
    const char* version = "v" GAME_VERSION;
    DrawTextEx(GetFontDefault(), version, (Vector2){ 10.0f, (float)(screenHeight - 25) }, 16, 1, DARKGRAY);
    
    // Title
    const char* title = "BoneBound";
    Vector2 titleSize = MeasureTextEx(GetFontDefault(), title, 48, 2);
    Vector2 titlePos = {
        screenWidth / 2.0f - titleSize.x / 2.0f,
        screenHeight / 4.0f
    };
    DrawTextEx(GetFontDefault(), title, titlePos, 48, 2, YELLOW);
    
    // Options
    const char* options[] = {"NEW GAME", "LOAD GAME", IsWindowFullscreen() ? "FULLSCREEN: ON" : "FULLSCREEN: OFF", "QUIT"};
    bool hasSaves = SaveFileExists(0) || SaveFileExists(1) || SaveFileExists(2);
    
    for (int i = 0; i < 4; i++) {
        Color color = (i == selectedOption) ? YELLOW : WHITE;
        
        if (i == 1 && !hasSaves)
            color = GRAY;
        
        Vector2 textSize = MeasureTextEx(GetFontDefault(), options[i], 32, 1);
        Vector2 textPos = {
            screenWidth / 2.0f - textSize.x / 2.0f,
            screenHeight / 2.0f + i * 50.0f
        };
        
        bool isGrayedOut = (i == 1 && !hasSaves);
        if (i == selectedOption && !isGrayedOut) {
            DrawText(">", textPos.x - 21, textPos.y, 32, YELLOW);
            DrawText("<", textPos.x + textSize.x + 10, textPos.y, 32, YELLOW);
        }
        
        DrawTextEx(GetFontDefault(), options[i], textPos, 32, 1, color);
    }
    
    // Instructions
    const char* inst = "UP/DOWN to navigate, ENTER to select";
    Vector2 instSize = MeasureTextEx(GetFontDefault(), inst, 20, 1);
    Vector2 instPos = {
        screenWidth / 2.0f - instSize.x / 2.0f,
        (float)screenHeight - 50.0f
    };
    DrawTextEx(GetFontDefault(), inst, instPos, 20, 1, LIGHTGRAY);
}

void MapSizeUpdate()
{
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        selectedOption = (selectedOption + 1) % NUM_SIZES;
    
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        selectedOption = (selectedOption - 1 + NUM_SIZES) % NUM_SIZES;
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        // Remember the chosen size, then move to character creation
        chosenMapSize = selectedOption;
        selectedOption = 0;                 // the character screen has its own 0..2 menu
        currentState = STATE_CHARACTER_CREATION;
        // Reset character creation variables
        strcpy(playerName, "Hero");
        nameCursorPos = 4;
        isEditingName = false;
        raceSelected = 1; // Default to Cat
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
        currentState = STATE_TITLE;
        selectedOption = 0;
    }
}

void MapSizeDraw()
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Version in bottom left
    const char* version = "v" GAME_VERSION;
    DrawTextEx(GetFontDefault(), version, (Vector2){ 10.0f, (float)(screenHeight - 25) }, 16, 1, DARKGRAY);
    
    // Title
    const char* title = "Map Size";
    Vector2 titleSize = MeasureTextEx(GetFontDefault(), title, 48, 2);
    Vector2 titlePos = {
        screenWidth / 2.0f - titleSize.x / 2.0f,
        screenHeight / 6.0f
    };
    DrawTextEx(GetFontDefault(), title, titlePos, 48, 2, YELLOW);
    
    // Selected info
    const char* info = TextFormat("Selected: %s (%dx%d)", 
        mapSizes[selectedOption].name,
        mapSizes[selectedOption].width,
        mapSizes[selectedOption].height);
    Vector2 infoSize = MeasureTextEx(GetFontDefault(), info, 22, 1);
    Vector2 infoPos = {
        screenWidth / 2.0f - infoSize.x / 2.0f,
        screenHeight / 3.0f
    };
    DrawTextEx(GetFontDefault(), info, infoPos, 22, 1, LIGHTGRAY);
    
    static const char* descs[NUM_SIZES] = {
        "A cosy island - quick to explore",
        "A small land with a handful of settlements",
        "A continent with rivers, forests and mountains",
        "Several landmasses, deserts and snowy peaks",
        "A large world with distinct climates",
        "A whole planet, pole to pole - huge (bigger saves, longer to generate)"
    };
    Vector2 dSize = MeasureTextEx(GetFontDefault(), descs[selectedOption], 18, 1);
    DrawTextEx(GetFontDefault(), descs[selectedOption], (Vector2){ screenWidth / 2.0f - dSize.x / 2.0f, screenHeight / 3.0f + 30 }, 18, 1, GRAY);

    // Options
    const char* names[] = {"TINY", "SMALL", "MEDIUM", "LARGE", "HUGE", "GIGANTIC"};
    
    for (int i = 0; i < NUM_SIZES; i++) {
        Color color = (i == selectedOption) ? YELLOW : WHITE;
        
        char text[50];
        sprintf(text, "%s (%dx%d)", names[i], mapSizes[i].width, mapSizes[i].height);
        
        Vector2 textSize = MeasureTextEx(GetFontDefault(), text, 28, 1);
        Vector2 textPos = {
            screenWidth / 2.0f - textSize.x / 2.0f,
            screenHeight / 2.0f + i * 40.0f
        };
        
        if (i == selectedOption) {
            DrawText(">", textPos.x - 21, textPos.y, 28, YELLOW);
            DrawText("<", textPos.x + textSize.x + 10, textPos.y, 28, YELLOW);
        }
        
        DrawTextEx(GetFontDefault(), text, textPos, 28, 1, color);
    }
    
    // Instructions
    const char* inst = "ENTER to continue, BACKSPACE to return";
    Vector2 instSize = MeasureTextEx(GetFontDefault(), inst, 16, 1);
    Vector2 instPos = {
        screenWidth / 2.0f - instSize.x / 2.0f,
        (float)screenHeight - 80.0f
    };
    DrawTextEx(GetFontDefault(), inst, instPos, 16, 1, LIGHTGRAY);
}

void CharacterCreationUpdate()
{
    if (!isEditingName) {
        // Navigation between options (Race selection)
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            selectedOption = (selectedOption + 1) % 3; // 0=Name, 1=Race, 2=Confirm
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            selectedOption = (selectedOption - 1 + 3) % 3;
        }
        
        // Enter sub-menus
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (selectedOption == 0) {
                // Start editing name
                isEditingName = true;
                nameCursorPos = strlen(playerName);
            } else if (selectedOption == 1) {
                // Toggle race
                raceSelected = (raceSelected + 1) % 2; // 0=Dog, 1=Cat
            } else if (selectedOption == 2) {
                // Confirm: reset the character, then generate the world on a background thread
                memset(&player, 0, sizeof(PlayerData));
                player.isCat = (raceSelected == 1);
                snprintf(player.name, sizeof(player.name), "%s", playerName);

                unsigned int seed = (unsigned int)time(NULL) * 2654435761u ^ (unsigned int)GetRandomValue(0, 0x7fffffff);
                BeginWorldGeneration(mapSizes[chosenMapSize].width, mapSizes[chosenMapSize].height, seed);
                currentState = STATE_GENERATING;
                selectedOption = 0;
            }
        }
    } else {
        // Name editing mode
        int key = GetCharPressed();
        
        // Handle backspace
        if (IsKeyPressed(KEY_BACKSPACE) && nameCursorPos > 0) {
            nameCursorPos--;
            playerName[nameCursorPos] = '\0';
        }
        
        // Handle character input
        while (key > 0) {
            if ((key >= 32) && (key <= 126) && nameCursorPos < 31) {
                playerName[nameCursorPos] = (char)key;
                nameCursorPos++;
                playerName[nameCursorPos] = '\0';
            }
            key = GetCharPressed();
        }
        
        // Press ENTER to finish editing
        if (IsKeyPressed(KEY_ENTER)) {
            if (nameCursorPos == 0) {
                // If name is empty, set default
                strcpy(playerName, "Hero");
                nameCursorPos = 4;
            }
            isEditingName = false;
        }
        
        // Press ESC to cancel editing (the game no longer quits on ESC, see main.cpp)
        if (IsKeyPressed(KEY_ESCAPE)) {
            strcpy(playerName, "Hero");
            nameCursorPos = 4;
            isEditingName = false;
        }
    }
    
    // Back to map size selection
    if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) && !isEditingName) {
        currentState = STATE_MAPSIZE;
        selectedOption = chosenMapSize;
    }
}

void CharacterCreationDraw()
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Version in bottom left
    const char* version = "v" GAME_VERSION;
    DrawTextEx(GetFontDefault(), version, (Vector2){ 10.0f, (float)(screenHeight - 25) }, 16, 1, DARKGRAY);
    
    // Title
    const char* title = "Character Creation";
    Vector2 titleSize = MeasureTextEx(GetFontDefault(), title, 48, 2);
    Vector2 titlePos = {
        screenWidth / 2.0f - titleSize.x / 2.0f,
        screenHeight / 6.0f
    };
    DrawTextEx(GetFontDefault(), title, titlePos, 48, 2, YELLOW);
    
    // ===== NAME SECTION =====
    int sectionSpacing = 60; // Space between sections
    
    // Name label
    const char* nameLabel = "Name:";
    Vector2 nameLabelSize = MeasureTextEx(GetFontDefault(), nameLabel, 24, 1);
    DrawTextEx(GetFontDefault(), nameLabel, 
        (Vector2){ screenWidth / 2.0f - 200.0f, screenHeight / 3.0f }, 24, 1, WHITE);
    
    // Name input box - 20 pixels to the right of the label
    Rectangle nameBox = { 
        screenWidth / 2.0f - 100.0f, 
        screenHeight / 3.0f - 5, 
        200, 
        30 
    };
    
    if (selectedOption == 0 && !isEditingName) {
        DrawRectangleLinesEx(nameBox, 2, YELLOW);
    } else {
        DrawRectangleLinesEx(nameBox, 1, LIGHTGRAY);
    }
    
    // Name text inside box
    DrawTextEx(GetFontDefault(), playerName, 
        (Vector2){ screenWidth / 2.0f - 95.0f, screenHeight / 3.0f }, 20, 1, WHITE);
    
    // Name cursor when editing
    if (isEditingName) {
        float cursorX = screenWidth / 2.0f - 95.0f + MeasureTextEx(GetFontDefault(), playerName, 20, 1).x;
        DrawTextEx(GetFontDefault(), "_", 
            (Vector2){ cursorX, screenHeight / 3.0f }, 20, 1, YELLOW);
    }
    
    // ===== RACE SECTION =====
    // Race label
    const char* raceLabel = "Race:";
    Vector2 raceLabelSize = MeasureTextEx(GetFontDefault(), raceLabel, 24, 1);
    DrawTextEx(GetFontDefault(), raceLabel, 
        (Vector2){ screenWidth / 2.0f - 200.0f, screenHeight / 3.0f + sectionSpacing }, 24, 1, WHITE);
    
    // Dog option - 20 pixels to the right of the label
    Color dogColor = (raceSelected == 0) ? YELLOW : WHITE;
    Vector2 dogPos = { screenWidth / 2.0f - 100.0f, screenHeight / 3.0f + sectionSpacing - 5 };
    DrawTextEx(GetFontDefault(), "Dog", dogPos, 28, 1, dogColor);
    
    // Cat option - 40 pixels to the right of Dog (20px gap)
    Color catColor = (raceSelected == 1) ? YELLOW : WHITE;
    Vector2 catPos = { screenWidth / 2.0f - 20.0f, screenHeight / 3.0f + sectionSpacing - 5 };
    DrawTextEx(GetFontDefault(), "Cat", catPos, 28, 1, catColor);
    
    // Race selector highlight boxes
    if (selectedOption == 1 && !isEditingName) {
        if (raceSelected == 0) {
            DrawRectangleLines(screenWidth / 2.0f - 105, screenHeight / 3.0f + sectionSpacing - 10, 60, 35, YELLOW);
        } else {
            DrawRectangleLines(screenWidth / 2.0f - 25, screenHeight / 3.0f + sectionSpacing - 10, 60, 35, YELLOW);
        }
    }
    
    // Race descriptions (centered under the options)
    const char* dogDesc = "Dogs are loyal and strong, with a keen nose";
    const char* catDesc = "Cats are agile and have night vision";
    
    float descX = screenWidth / 2.0f - 200.0f;
    float descY = screenHeight / 3.0f + sectionSpacing + 30;
    
    if (raceSelected == 0) {
        DrawTextEx(GetFontDefault(), dogDesc, (Vector2){ descX, descY }, 14, 1, LIGHTGRAY);
    } else {
        DrawTextEx(GetFontDefault(), catDesc, (Vector2){ descX, descY }, 14, 1, LIGHTGRAY);
    }
    
    // ===== CONFIRM SECTION =====
    // Confirm button - centered with more spacing
    Color confirmColor = (selectedOption == 2 && !isEditingName) ? YELLOW : WHITE;
    Vector2 confirmPos = { screenWidth / 2.0f - 80.0f, screenHeight / 3.0f + sectionSpacing * 2 + 20 };
    DrawTextEx(GetFontDefault(), "Start Adventure", confirmPos, 28, 1, confirmColor);
    
    if (selectedOption == 2 && !isEditingName) {
        DrawText(">", confirmPos.x - 30, confirmPos.y, 28, YELLOW);
        DrawText("<", confirmPos.x + 190, confirmPos.y, 28, YELLOW);
    }
    
    // ===== INSTRUCTIONS =====
    const char* editInst = isEditingName ? 
        "Type your name, ENTER to confirm, ESC to cancel" : 
        "UP/DOWN to navigate, ENTER to select, BACKSPACE to go back";
    Vector2 instSize = MeasureTextEx(GetFontDefault(), editInst, 16, 1);
    Vector2 instPos = {
        screenWidth / 2.0f - instSize.x / 2.0f,
        (float)screenHeight - 80.0f
    };
    DrawTextEx(GetFontDefault(), editInst, instPos, 16, 1, LIGHTGRAY);
}

void SaveMenuUpdate()
{
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        saveSlotSelected = (saveSlotSelected + 1) % 3;
    
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        saveSlotSelected = (saveSlotSelected - 1 + 3) % 3;
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (SaveGame(saveSlotSelected)) {
            snprintf(menuStatus, sizeof(menuStatus), "Game saved to slot %d", saveSlotSelected + 1);
            menuStatusUntil = GetTime() + 2.5;
        } else {
            snprintf(menuStatus, sizeof(menuStatus), "Saving failed!");
            menuStatusUntil = GetTime() + 3.0;
        }
        currentState = (menuReturnState == STATE_PAUSE) ? STATE_PAUSE : STATE_PLAYING;
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
        currentState = (worldMap != NULL) ? menuReturnState : STATE_TITLE;
        if (currentState == STATE_TITLE) selectedOption = 0;
    }
}

void SaveMenuDraw()
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Version in bottom left
    const char* version = "v" GAME_VERSION;
    DrawTextEx(GetFontDefault(), version, (Vector2){ 10.0f, (float)(screenHeight - 25) }, 16, 1, DARKGRAY);
    
    // Title
    const char* title = "Save Game";
    Vector2 titleSize = MeasureTextEx(GetFontDefault(), title, 48, 2);
    Vector2 titlePos = {
        screenWidth / 2.0f - titleSize.x / 2.0f,
        screenHeight / 6.0f
    };
    DrawTextEx(GetFontDefault(), title, titlePos, 48, 2, YELLOW);
    
    // Slots
    const char* slots[] = {"Save Slot 1", "Save Slot 2", "Save Slot 3"};
    
    for (int i = 0; i < 3; i++) {
        Color color = (i == saveSlotSelected) ? YELLOW : WHITE;
        
        // Check if save exists to show character info
        bool exists = SaveFileExists(i);
        
        Vector2 textSize = MeasureTextEx(GetFontDefault(), slots[i], 32, 1);
        Vector2 textPos = {
            screenWidth / 2.0f - textSize.x / 2.0f,
            screenHeight / 2.0f + i * 60.0f
        };
        
        if (i == saveSlotSelected) {
            DrawText(">", textPos.x - 21, textPos.y, 32, YELLOW);
            DrawText("<", textPos.x + textSize.x + 10, textPos.y, 32, YELLOW);
        }
        
        DrawTextEx(GetFontDefault(), slots[i], textPos, 32, 1, color);
        
        if (exists) {
            SaveInfo info;
            const char* line = ReadSaveInfo(i, &info) ?
                TextFormat("%s the %s - Lv %d - %s  (will be overwritten)", info.name, info.isCat ? "Cat" : "Dog", info.level, info.locationName)
                : "(incompatible save - will be overwritten)";
            Vector2 lsz = MeasureTextEx(GetFontDefault(), line, 16, 1);
            DrawTextEx(GetFontDefault(), line, (Vector2){ screenWidth / 2.0f - lsz.x / 2.0f, textPos.y + 34.0f }, 16, 1, GRAY);
        } else {
            Vector2 lsz = MeasureTextEx(GetFontDefault(), "(Empty)", 16, 1);
            DrawTextEx(GetFontDefault(), "(Empty)", (Vector2){ screenWidth / 2.0f - lsz.x / 2.0f, textPos.y + 34.0f }, 16, 1, GRAY);
        }
    }
}

void LoadMenuUpdate()
{
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        saveSlotSelected = (saveSlotSelected + 1) % 3;
    
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        saveSlotSelected = (saveSlotSelected - 1 + 3) % 3;
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (SaveFileExists(saveSlotSelected)) {
            if (LoadGame(saveSlotSelected)) {
                currentState = STATE_PLAYING;
            } else {
                snprintf(menuStatus, sizeof(menuStatus), "Could not load slot %d (old or damaged save)", saveSlotSelected + 1);
                menuStatusUntil = GetTime() + 3.5;
            }
        }
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
        currentState = (menuReturnState == STATE_TITLE || worldMap == NULL) ? STATE_TITLE : menuReturnState;
        if (currentState == STATE_TITLE) selectedOption = 0;
    }
}

void LoadMenuDraw()
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    if (GetTime() < menuStatusUntil)
        DrawTextEx(GetFontDefault(), menuStatus, (Vector2){ 20.0f, (float)(screenHeight - 55) }, 20, 1, ORANGE);
    
    // Version in bottom left
    const char* version = "v" GAME_VERSION;
    DrawTextEx(GetFontDefault(), version, (Vector2){ 10.0f, (float)(screenHeight - 25) }, 16, 1, DARKGRAY);
    
    // Title
    const char* title = "Load Game";
    Vector2 titleSize = MeasureTextEx(GetFontDefault(), title, 48, 2);
    Vector2 titlePos = {
        screenWidth / 2.0f - titleSize.x / 2.0f,
        screenHeight / 6.0f
    };
    DrawTextEx(GetFontDefault(), title, titlePos, 48, 2, YELLOW);
    
    // Slots
    const char* slots[] = {"Load Slot 1", "Load Slot 2", "Load Slot 3"};
    
    for (int i = 0; i < 3; i++) {
        bool exists = SaveFileExists(i);
        Color color = (i == saveSlotSelected && exists) ? YELLOW : (exists ? WHITE : GRAY);
        
        Vector2 textSize = MeasureTextEx(GetFontDefault(), slots[i], 32, 1);
        Vector2 textPos = {
            screenWidth / 2.0f - textSize.x / 2.0f,
            screenHeight / 2.0f + i * 60.0f
        };
        
        if (i == saveSlotSelected && exists) {
            DrawText(">", textPos.x - 21, textPos.y, 32, YELLOW);
            DrawText("<", textPos.x + textSize.x + 10, textPos.y, 32, YELLOW);
        }
        
        DrawTextEx(GetFontDefault(), slots[i], textPos, 32, 1, color);
        
        if (!exists) {
            Vector2 emptySize = MeasureTextEx(GetFontDefault(), "(Empty)", 20, 1);
            Vector2 emptyPos = {
                screenWidth / 2.0f - emptySize.x / 2.0f,
                textPos.y + 35.0f
            };
            DrawTextEx(GetFontDefault(), "(Empty)", emptyPos, 20, 1, GRAY);
        } else {
            SaveInfo info;
            const char* line = ReadSaveInfo(i, &info) ?
                TextFormat("%s the %s - Lv %d - %s (%dx%d)", info.name, info.isCat ? "Cat" : "Dog", info.level, info.locationName, info.worldWidth, info.worldHeight)
                : "(incompatible save - made by an older version)";
            Vector2 lsz = MeasureTextEx(GetFontDefault(), line, 16, 1);
            DrawTextEx(GetFontDefault(), line, (Vector2){ screenWidth / 2.0f - lsz.x / 2.0f, textPos.y + 34.0f }, 16, 1, LIGHTGRAY);
        }
    }
}

// ============================================================================
// PAUSE MENU
// ============================================================================
void PauseUpdate()
{
    static const int COUNT = 6;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) selectedOption = (selectedOption + 1) % COUNT;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))   selectedOption = (selectedOption - 1 + COUNT) % COUNT;

    if (IsKeyPressed(KEY_ESCAPE)) { currentState = STATE_PLAYING; return; }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (selectedOption) {
            case 0: currentState = STATE_PLAYING; break;
            case 1: menuReturnState = STATE_PAUSE; saveSlotSelected = 0; currentState = STATE_SAVE_MENU; break;
            case 2: menuReturnState = STATE_PAUSE; saveSlotSelected = 0; currentState = STATE_LOAD_MENU; break;
            case 3: ToggleFullscreen(SCREEN_WIDTH, SCREEN_HEIGHT); break;
            case 4: CleanupAllMaps(); currentState = STATE_TITLE; selectedOption = 0; break;
            case 5: shouldQuit = true; break;
        }
    }
}

void PauseDraw()
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 170 });

    const char* title = "Paused";
    Vector2 ts = MeasureTextEx(GetFontDefault(), title, 48, 2);
    DrawTextEx(GetFontDefault(), title, (Vector2){ sw / 2.0f - ts.x / 2.0f, sh / 5.0f }, 48, 2, YELLOW);

    const char* options[] = { "RESUME", "SAVE GAME", "LOAD GAME", IsWindowFullscreen() ? "FULLSCREEN: ON" : "FULLSCREEN: OFF", "QUIT TO TITLE", "QUIT GAME" };
    for (int i = 0; i < 6; i++) {
        Color c = (i == selectedOption) ? YELLOW : WHITE;
        Vector2 sz = MeasureTextEx(GetFontDefault(), options[i], 30, 1);
        Vector2 pos = { sw / 2.0f - sz.x / 2.0f, sh / 2.5f + i * 46.0f };
        if (i == selectedOption) {
            DrawText(">", (int)pos.x - 24, (int)pos.y, 30, YELLOW);
            DrawText("<", (int)(pos.x + sz.x + 12), (int)pos.y, 30, YELLOW);
        }
        DrawTextEx(GetFontDefault(), options[i], pos, 30, 1, c);
    }

    if (selectedOption == 4) {
        const char* warn = "Unsaved progress will be lost";
        Vector2 ws = MeasureTextEx(GetFontDefault(), warn, 18, 1);
        DrawTextEx(GetFontDefault(), warn, (Vector2){ sw / 2.0f - ws.x / 2.0f, sh / 2.5f + 6 * 46.0f + 10 }, 18, 1, ORANGE);
    }
    if (GetTime() < menuStatusUntil)
        DrawTextEx(GetFontDefault(), menuStatus, (Vector2){ 20.0f, (float)(sh - 55) }, 20, 1, GREEN);

    DrawTextEx(GetFontDefault(), TextFormat("World seed: %u", worldSeed), (Vector2){ 20.0f, (float)(sh - 28) }, 16, 1, GRAY);
}

// ============================================================================
// WORLD GENERATION LOADING SCREEN
// ============================================================================
void GeneratingUpdate()
{
    if (UpdateWorldGeneration()) {
        currentState = STATE_CHOOSE_START;      // the world exists: now the player picks where to begin
    }
}

void GeneratingDraw()
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    const char* title = "Creating your world...";
    Vector2 ts = MeasureTextEx(GetFontDefault(), title, 40, 2);
    DrawTextEx(GetFontDefault(), title, (Vector2){ sw / 2.0f - ts.x / 2.0f, sh / 3.0f }, 40, 2, YELLOW);

    int progress = WorldGen_GetProgress();
    float barW = sw * 0.5f, barH = 26.0f;
    float bx = sw / 2.0f - barW / 2.0f, by = sh / 2.0f;
    DrawRectangleLinesEx((Rectangle){ bx, by, barW, barH }, 2, LIGHTGRAY);
    DrawRectangle((int)bx + 4, (int)by + 4, (int)((barW - 8) * progress / 100.0f), (int)barH - 8, (Color){ 100, 200, 100, 255 });

    const char* stage = WorldGen_GetStageName();
    Vector2 ss = MeasureTextEx(GetFontDefault(), stage, 22, 1);
    DrawTextEx(GetFontDefault(), stage, (Vector2){ sw / 2.0f - ss.x / 2.0f, by + barH + 18 }, 22, 1, LIGHTGRAY);
    DrawTextEx(GetFontDefault(), TextFormat("%d%%   seed %u", progress, worldSeed), (Vector2){ sw / 2.0f - 70.0f, by + barH + 52 }, 16, 1, GRAY);
}


// ============================================================================
// CHOOSE YOUR STARTING LOCATION
// While picking, player.worldPos is the cursor and previewWholeMap shows the entire world.
// ============================================================================
static char pickMessage[96] = "";
static double pickMessageUntil = 0.0;

static bool NextToOcean(int x, int y)
{
    for (int oy = -1; oy <= 1; oy++)
        for (int ox = -1; ox <= 1; ox++) {
            int nx = x + ox, ny = y + oy;
            if (nx < 0 || ny < 0 || nx >= currentMapWidth || ny >= currentMapHeight) continue;
            BiomeType b = worldMap[ny][nx].biome;
            if (b == BIOME_OCEAN || b == BIOME_DEEP_OCEAN) return true;
        }
    return false;
}

void StartPickUpdate()
{
    // --- move the cursor (hold SHIFT to move 5 tiles at a time) ---
    int dx = 0, dy = 0;
    if (ReadMoveStep(&dx, &dy, 0.20f, 0.03f)) {
        int step = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) ? 5 : 1;
        int nx = player.worldPos.x + dx * step, ny = player.worldPos.y + dy * step;
        if (nx < 0) nx = 0;
        if (ny < 0) ny = 0;
        if (nx >= currentMapWidth)  nx = currentMapWidth - 1;
        if (ny >= currentMapHeight) ny = currentMapHeight - 1;
        player.worldPos.x = nx;
        player.worldPos.y = ny;
        pickMessage[0] = '\0';
    }

    // --- R: jump back to the recommended start ---
    if (IsKeyPressed(KEY_R)) {
        int rx, ry;
        GetRecommendedStart(&rx, &ry);
        player.worldPos.x = rx;
        player.worldPos.y = ry;
    }

    // --- Z: toggle between "whole world" and a close-up around the cursor ---
    if (IsKeyPressed(KEY_Z)) {
        gameCamera.zoom = (gameCamera.camera.zoom < 0.5f) ? 1.0f : FitWorldZoom();
        gameCamera.camera.zoom = gameCamera.zoom;
    }

    // --- ENTER: settle here (open water is not allowed) ---
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (WorldTileEnterable(player.worldPos.x, player.worldPos.y)) {
            FinalizeNewWorld(player.worldPos.x, player.worldPos.y);   // sets stats, reveals the surroundings, resets the camera
            currentState = STATE_PLAYING;
            return;
        }
        snprintf(pickMessage, sizeof(pickMessage), "You can't start in open water - pick a spot on land");
        pickMessageUntil = GetTime() + 2.5;
    }

    // --- ESC / BACKSPACE: throw this world away and go back ---
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
        previewWholeMap = false;
        CleanupAllMaps();
        currentState = STATE_TITLE;
        selectedOption = 0;
        return;
    }

    UpdateCamera();     // follows the cursor and handles the mouse wheel

    // when the whole map fits on screen, keep it centred instead of chasing the cursor
    float z = gameCamera.camera.zoom;
    if (currentMapWidth * TILE_SIZE * z <= GetScreenWidth())   gameCamera.camera.target.x = currentMapWidth * TILE_SIZE / 2.0f;
    if (currentMapHeight * TILE_SIZE * z <= GetScreenHeight()) gameCamera.camera.target.y = currentMapHeight * TILE_SIZE / 2.0f;
}

static void DrawStartCursor()
{
    float zoom = gameCamera.camera.zoom;
    float size = fmaxf((float)TILE_SIZE, 16.0f / zoom);          // stays visible when zoomed far out
    float th = 2.5f / zoom;
    float cx = player.worldPos.x * TILE_SIZE + TILE_SIZE * 0.5f;
    float cy = player.worldPos.y * TILE_SIZE + TILE_SIZE * 0.5f;
    bool ok = WorldTileEnterable(player.worldPos.x, player.worldPos.y);
    unsigned char pulse = (unsigned char)(170 + 85 * sinf((float)GetTime() * 6.0f));
    Color c = ok ? (Color){ 255, 230, 60, pulse } : (Color){ 255, 80, 80, pulse };
    DrawRectangleLinesEx((Rectangle){ cx - size / 2 - th, cy - size / 2 - th, size + 2 * th, size + 2 * th }, th, c);
}

void StartPickDraw()
{
    BeginMode2D(gameCamera.camera);
    DrawWorldMap();
    DrawStartCursor();
    EndMode2D();

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    const WorldTile* t = &worldMap[player.worldPos.y][player.worldPos.x];
    bool ok = WorldTileEnterable(player.worldPos.x, player.worldPos.y);

    // title strip
    DrawRectangle(0, 0, sw, 44, (Color){ 0, 0, 0, 190 });
    const char* title = "Choose Your Starting Location";
    Vector2 ts = MeasureTextEx(GetFontDefault(), title, 28, 1);
    DrawTextEx(GetFontDefault(), title, (Vector2){ sw / 2.0f - ts.x / 2.0f, 8 }, 28, 1, GOLD);

    // info panel for the tile under the cursor
    DrawRectangle(0, sh - 110, sw, 110, (Color){ 0, 0, 0, 200 });
    const char* typeStr = LocationTypeName(t->locationType);
    DrawText(t->locationName, 14, sh - 102, 24, ok ? YELLOW : (Color){ 255, 120, 120, 255 });

    char detail[200];
    snprintf(detail, sizeof(detail), "%s%s%s  -  %s%s%s",
             typeStr ? typeStr : "Wilderness", typeStr ? ", " : "", BiomeName(t->biome),
             TemperatureWord(t->temperature),
             (t->riverMask && !t->isLake) ? ", river" : "",
             (ok && NextToOcean(player.worldPos.x, player.worldPos.y)) ? ", coastal" : "");
    DrawText(detail, 14, sh - 72, 18, LIGHTGRAY);

    if (GetTime() < pickMessageUntil)
        DrawText(pickMessage, 14, sh - 46, 18, (Color){ 255, 120, 120, 255 });
    else if (!ok)
        DrawText("Open water - you can't start here", 14, sh - 46, 18, (Color){ 255, 120, 120, 255 });
    else if (t->locationType == LOCATION_VILLAGE)
        DrawText("A village: friendly folk, a safe place to begin", 14, sh - 46, 18, (Color){ 140, 230, 140, 255 });
    else if (t->locationType == LOCATION_DUNGEON || t->locationType == LOCATION_CAVE)
        DrawText("Dangerous: a risky place to begin!", 14, sh - 46, 18, ORANGE);
    else if (t->locationType == LOCATION_NONE)
        DrawText("Open wilderness - you'll have to fend for yourself", 14, sh - 46, 18, LIGHTGRAY);

    DrawText("Arrows/WASD: move (SHIFT = fast)   Wheel/Z: zoom   R: recommended start   ENTER: begin here   ESC: cancel",
             14, sh - 22, 16, GRAY);
    DrawText(TextFormat("seed %u   (%d,%d)", worldSeed, player.worldPos.x, player.worldPos.y), sw - 230, sh - 100, 16, GRAY);
}
