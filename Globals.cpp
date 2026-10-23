// Globals.cpp (in root folder, not in Systems/)
#include "project.h"

// ---- Logger (single shared handle; see Systems/Logger.h) ----
FILE* logFile = NULL;

void InitLogger()
{
    if (logFile) return;
    logFile = fopen("debug_log.txt", "w");
    if (logFile) {
        time_t now = time(NULL);
        fprintf(logFile, "=== BONEBOUND DEBUG LOG ===\n");
        fprintf(logFile, "Version: %s\n", GAME_VERSION);
        fprintf(logFile, "Started: %s", ctime(&now));
        fflush(logFile);
    }
}

void CloseLogger()
{
    if (logFile) {
        time_t now = time(NULL);
        fprintf(logFile, "\nClosed: %s", ctime(&now));
        fclose(logFile);
        logFile = NULL;
    }
}

// Define all global variables here
WorldTile** worldMap = NULL;
GameState currentState = STATE_TITLE;
PlayerData player = {{2, 2}, {2, 2}, false};
GameCamera gameCamera = {0};
NPC** npcs = NULL;
int npcCount = 0;
int currentMapWidth = DEFAULT_WORLD_WIDTH;
int currentMapHeight = DEFAULT_WORLD_HEIGHT;
bool shouldQuit = false;
unsigned int worldSeed = 0;
bool previewWholeMap = false;   // true while choosing a start: the whole map is drawn as if explored
int selectedOption = 0;
int saveSlotSelected = 0;
NPC* talkingNPC = NULL;
bool isInConversation = false;
int conversationChoice = 0;

// Map configurations
MapConfig mapSizes[NUM_SIZES] = {
    {16, 16, "TINY", 4.0f},
    {32, 32, "SMALL", 2.0f},
    {64, 64, "MEDIUM", 1.5f},
    {128, 128, "LARGE", 0.8f},
    {256, 256, "HUGE", 0.4f},
    {512, 512, "GIGANTIC", 0.2f}
};
