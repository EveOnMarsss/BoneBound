// FIRST: Include our wrapper (this handles windows.h with renamed symbols)
#include "WindowsWrapper.h"

// THEN: Include standard headers
#include <stdio.h>
#include <time.h>

// LAST: Include project.h (which includes raylib)
#include "../project.h"
#include "../World/LocalMap.h"  // Add this for LocalMap definition

static int crashInProgress = 0;

// Helper function to get location type name
const char* GetLocationTypeName(LocationType type)
{
    switch (type) {
        case LOCATION_NONE: return "NONE (Generic Terrain)";
        case LOCATION_VILLAGE: return "VILLAGE";
        case LOCATION_CASTLE: return "CASTLE";
        case LOCATION_CAVE: return "CAVE";
        case LOCATION_WIZARD_TOWER: return "WIZARD_TOWER";
        case LOCATION_DUNGEON: return "DUNGEON";
        case LOCATION_RUINS: return "RUINS";
        case LOCATION_FORT: return "FORT";
        case LOCATION_TEMPLE: return "TEMPLE";
        default: return "UNKNOWN";
    }
}

// Message box functions
void ShowErrorBox(const char* message, const char* title)
{
    MessageBoxA(NULL, message, title, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

void ShowWarningBox(const char* message, const char* title)
{
    MessageBoxA(NULL, message, title, MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
}

// Get detailed explanation for different exception types
const char* GetExceptionExplanation(DWORD code, EXCEPTION_POINTERS* ep)
{
    static char buffer[1024];
    
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
        {
            BOOL isWrite = ep->ExceptionRecord->ExceptionInformation[0];
            void* accessAddr = (void*)ep->ExceptionRecord->ExceptionInformation[1];
            
            sprintf(buffer, 
                "ACCESS VIOLATION - The game tried to %s memory at address %p.\n\n"
                "COMMON CAUSES:\n"
                "• Dereferencing a NULL or uninitialized pointer\n"
                "• Accessing memory that has already been freed\n"
                "• Array index out of bounds\n"
                "• Using a pointer after the object it points to has been destroyed\n"
                "• Stack overflow from infinite recursion\n\n"
                "CHECK FOR:\n"
                "• worldMap[y][x] access without bounds checking\n"
                "• NPC array access with invalid index\n"
                "• Player position being used before validation",
                isWrite ? "write to" : "read from", accessAddr);
            return buffer;
        }
        
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            sprintf(buffer,
                "ARRAY BOUNDS EXCEEDED - The game tried to access an array index outside its valid range.\n\n"
                "COMMON CAUSES:\n"
                "• Using an index >= array size\n"
                "• Off-by-one errors in loops (using <= instead of <)\n"
                "• Not validating input before using it as an array index\n\n"
                "CHECK FOR:\n"
                "• Loop conditions in map rendering\n"
                "• NPC array access without checking npcCount\n"
                "• Tile coordinate calculations");
            return buffer;
            
        case EXCEPTION_STACK_OVERFLOW:
            sprintf(buffer,
                "STACK OVERFLOW - The game ran out of stack space.\n\n"
                "COMMON CAUSES:\n"
                "• Infinite recursion (a function calling itself forever)\n"
                "• Allocating very large local variables on the stack\n"
                "• Deeply nested function calls\n\n"
                "CHECK FOR:\n"
                "• Recursive functions without proper termination\n"
                "• Large arrays declared as local variables\n"
                "• Excessive call depth in pathfinding or map generation");
            return buffer;
            
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            sprintf(buffer,
                "DIVIDE BY ZERO - The game tried to divide a number by zero.\n\n"
                "COMMON CAUSES:\n"
                "• Using uninitialized variable as divisor\n"
                "• Not checking if a value is zero before division\n"
                "• Mathematical formulas that can result in zero denominator\n\n"
                "CHECK FOR:\n"
                "• Map size calculations\n"
                "• Percentage or ratio calculations\n"
                "• Anywhere division is used");
            return buffer;
            
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            sprintf(buffer,
                "ILLEGAL INSTRUCTION - The CPU tried to execute an invalid instruction.\n\n"
                "COMMON CAUSES:\n"
                "• Jumping to an invalid code address (corrupted function pointer)\n"
                "• Stack corruption causing return to invalid address\n"
                "• Buffer overflow overwriting code or return addresses\n\n"
                "THIS OFTEN INDICATES MEMORY CORRUPTION.");
            return buffer;
            
        default:
            sprintf(buffer,
                "UNKNOWN EXCEPTION (Code: 0x%08X)\n\n"
                "This is an unexpected error type. The crash log contains technical details\n"
                "that can help the developer diagnose the issue.",
                code);
            return buffer;
    }
}

// Get detailed explanation for signal types
const char* GetSignalExplanation(int signal)
{
    static char buffer[1024];
    
    switch (signal) {
        case SIGSEGV:
            sprintf(buffer,
                "SEGMENTATION FAULT - The game tried to access memory it doesn't have permission to access.\n\n"
                "COMMON CAUSES:\n"
                "• Dereferencing a NULL pointer (trying to use memory address 0)\n"
                "• Using a pointer after the memory has been freed\n"
                "• Accessing an array index outside its bounds\n"
                "• Stack overflow from too many nested function calls\n\n"
                "CHECK YOUR CODE FOR:\n"
                "• worldMap[y][x] access without bounds checking (y=%d, x=%d)\n"
                "• NPC array access with index >= npcCount (%d)\n"
                "• Player position validation",
                player.worldPos.y, player.worldPos.x, npcCount);
            return buffer;
            
        case SIGABRT:
            sprintf(buffer,
                "ABORT - The program deliberately terminated itself.\n\n"
                "COMMON CAUSES:\n"
                "• Assertion failure (assert() macro triggered)\n"
                "• Detected internal inconsistency in the program\n"
                "• Critical error that the program couldn't recover from\n"
                "• Memory allocation failure (out of memory)\n\n"
                "This usually indicates a bug that was detected by internal checks.");
            return buffer;
            
        case SIGFPE:
            sprintf(buffer,
                "FLOATING POINT EXCEPTION - Mathematical error in calculation.\n\n"
                "COMMON CAUSES:\n"
                "• Integer division by zero\n"
                "• Floating point overflow (number too large to represent)\n"
                "• Invalid mathematical operation\n\n"
                "CHECK FOR:\n"
                "• Division operations without zero checks\n"
                "• Map scaling calculations\n"
                "• Camera zoom calculations");
            return buffer;
            
        case SIGILL:
            sprintf(buffer,
                "ILLEGAL INSTRUCTION - The CPU encountered an instruction it doesn't understand.\n\n"
                "COMMON CAUSES:\n"
                "• Corrupted code in memory (buffer overflow)\n"
                "• Jumping to an invalid function pointer\n"
                "• Stack corruption\n"
                "• Trying to execute data as code\n\n"
                "THIS OFTEN INDICATES SERIOUS MEMORY CORRUPTION.");
            return buffer;
            
        default:
            sprintf(buffer, "Unknown signal %d.\n\nCheck crashlog.txt for technical details.", signal);
            return buffer;
    }
}

// Save detailed crash info
void SaveDetailedCrashLog(const char* errorType, const char* details, EXCEPTION_POINTERS* ep)
{
    FILE* log = fopen("crashlog.txt", "w");
    if (!log) return;
    
    time_t now = time(NULL);
    
    fprintf(log, "========================================\n");
    fprintf(log, "     BONEBOUND CRASH REPORT\n");
    fprintf(log, "========================================\n");
    fprintf(log, "Time: %s", ctime(&now));
    fprintf(log, "Error Type: %s\n", errorType);
    
    if (ep) {
        fprintf(log, "Exception Code: 0x%08X\n", ep->ExceptionRecord->ExceptionCode);
        fprintf(log, "Exception Address: %p\n", ep->ExceptionRecord->ExceptionAddress);
        fprintf(log, "Exception Flags: 0x%X\n", ep->ExceptionRecord->ExceptionFlags);
        fprintf(log, "Number Parameters: %d\n", ep->ExceptionRecord->NumberParameters);
    }
    
    fprintf(log, "\n========================================\n");
    fprintf(log, "         GAME STATE\n");
    fprintf(log, "========================================\n");
    fprintf(log, "Current State: %d ", currentState);
    switch(currentState) {
        case 0: fprintf(log, "(STATE_TITLE)\n"); break;
        case 1: fprintf(log, "(STATE_MAPSIZE)\n"); break;
        case 2: fprintf(log, "(STATE_SEED)\n"); break;
        case 3: fprintf(log, "(STATE_PLAYING)\n"); break;
        case 4: fprintf(log, "(STATE_SAVE_MENU)\n"); break;
        case 5: fprintf(log, "(STATE_LOAD_MENU)\n"); break;
        default: fprintf(log, "(UNKNOWN)\n"); break;
    }
    fprintf(log, "In Local Map: %s\n", player.isInLocalMap ? "Yes" : "No");
    fprintf(log, "World Position: (%d,%d)\n", player.worldPos.x, player.worldPos.y);
    fprintf(log, "Local Position: (%d,%d)\n", player.localPos.x, player.localPos.y);
    fprintf(log, "Total NPCs: %d\n", npcCount);
    fprintf(log, "Map Size: %dx%d\n", currentMapWidth, currentMapHeight);
    fprintf(log, "worldMap pointer: %p\n", (void*)worldMap);
    
    // Check for common issues
    fprintf(log, "\n========================================\n");
    fprintf(log, "         DIAGNOSTIC CHECKS\n");
    fprintf(log, "========================================\n");
    
    if (!worldMap) {
        fprintf(log, "❌ worldMap is NULL - map not initialized!\n");
    } else {
        fprintf(log, "✓ worldMap is valid\n");
        
        // Check player world position
        if (player.worldPos.y < 0 || player.worldPos.y >= currentMapHeight ||
            player.worldPos.x < 0 || player.worldPos.x >= currentMapWidth) {
            fprintf(log, "❌ World position (%d,%d) is OUT OF BOUNDS for map %dx%d\n",
                   player.worldPos.x, player.worldPos.y, currentMapWidth, currentMapHeight);
        } else {
            fprintf(log, "✓ World position is within bounds\n");
            
            // Try to read the tile with FULL details
            WorldTile* tile = &worldMap[player.worldPos.y][player.worldPos.x];
            fprintf(log, "\n  CURRENT TILE DETAILS:\n");
            fprintf(log, "  • Terrain Type: '%c'\n", tile->worldTile);
            fprintf(log, "  • Has Local Map: %s\n", tile->hasLocalMap ? "Yes" : "No");
            fprintf(log, "  • Location Type ID: %d\n", tile->locationType);
            fprintf(log, "  • Location Type Name: %s\n", GetLocationTypeName(tile->locationType));
            fprintf(log, "  • Location Name: \"%s\"\n", tile->locationName);
            fprintf(log, "  • Discovered: %s\n", tile->discovered ? "Yes" : "No");
            fprintf(log, "  • Has Exit Point: %s\n", tile->hasExitPoint ? "Yes" : "No");
            fprintf(log, "  • Exit Point: (%d,%d)\n", tile->lastExitX, tile->lastExitY);
            
            // If it's a location, check if local map exists
            if (tile->locationType != LOCATION_NONE) {
                fprintf(log, "\n  LOCAL MAP STATUS:\n");
                if (tile->localMap) {
                    fprintf(log, "  ✓ Local map exists\n");
                    fprintf(log, "  • Buildings: %d\n", tile->localMap->buildingCount);
                    fprintf(log, "  • Spawn Points: %d\n", tile->localMap->spawnPointCount);
                    
                    // Check first few tiles to verify map data
                    if (tile->localMap->tiles) {
                        fprintf(log, "  • Sample tiles (10,10): '%c'\n", 
                               tile->localMap->tiles[10][10]);
                    }
                } else {
                    fprintf(log, "  ❌ Local map is NULL!\n");
                }
            }
        }
    }
    
    // Check local position
    if (player.isInLocalMap) {
        fprintf(log, "\n  LOCAL POSITION CHECK:\n");
        if (player.localPos.x < 0 || player.localPos.x >= LOCAL_MAP_WIDTH ||
            player.localPos.y < 0 || player.localPos.y >= LOCAL_MAP_HEIGHT) {
            fprintf(log, "  ❌ Local position (%d,%d) is OUT OF BOUNDS for local map %dx%d\n",
                   player.localPos.x, player.localPos.y, LOCAL_MAP_WIDTH, LOCAL_MAP_HEIGHT);
        } else {
            fprintf(log, "  ✓ Local position is within bounds\n");
            
            // Try to get the tile at local position
            WorldTile* tile = &worldMap[player.worldPos.y][player.worldPos.x];
            if (tile->localMap && tile->localMap->tiles) {
                char localTile = tile->localMap->tiles[player.localPos.y][player.localPos.x];
                fprintf(log, "  • Tile at local position: '%c'\n", localTile);
            }
        }
    } else {
        fprintf(log, "\n  ℹ Not in local map - local position (%d,%d) should not be accessed\n",
               player.localPos.x, player.localPos.y);
    }
    
    // Check NPCs
    fprintf(log, "\n  NPC STATUS:\n");
    if (npcCount > 0) {
        fprintf(log, "  ✓ %d NPCs loaded\n", npcCount);
        if (npcCount > 500) {
            fprintf(log, "  ⚠ Warning: Very high NPC count (%d) may cause performance issues\n", npcCount);
        }
        
        // Count NPCs by type in current map
        int villagers = 0, guards = 0, merchants = 0, innkeepers = 0, 
            blacksmiths = 0, wanderers = 0, bandits = 0, wizards = 0;
            
        for (int i = 0; i < npcCount; i++) {
            if (npcs[i] && npcs[i]->worldX == player.worldPos.x && 
                npcs[i]->worldY == player.worldPos.y) {
                switch(npcs[i]->type) {
                    case NPC_VILLAGER: villagers++; break;
                    case NPC_GUARD: guards++; break;
                    case NPC_MERCHANT: merchants++; break;
                    case NPC_INN_KEEPER: innkeepers++; break;
                    case NPC_BLACKSMITH: blacksmiths++; break;
                    case NPC_WANDERER: wanderers++; break;
                    case NPC_BANDIT: bandits++; break;
                    case NPC_WIZARD: wizards++; break;
                    default: break;
                }
            }
        }
        
        if (villagers > 0) fprintf(log, "  • Villagers: %d\n", villagers);
        if (guards > 0) fprintf(log, "  • Guards: %d\n", guards);
        if (merchants > 0) fprintf(log, "  • Merchants: %d\n", merchants);
        if (innkeepers > 0) fprintf(log, "  • Innkeepers: %d\n", innkeepers);
        if (blacksmiths > 0) fprintf(log, "  • Blacksmiths: %d\n", blacksmiths);
        if (wanderers > 0) fprintf(log, "  • Wanderers: %d\n", wanderers);
        if (bandits > 0) fprintf(log, "  • Bandits: %d\n", bandits);
        if (wizards > 0) fprintf(log, "  • Wizards: %d\n", wizards);
        
        int totalInMap = villagers + guards + merchants + innkeepers + 
                        blacksmiths + wanderers + bandits + wizards;
        fprintf(log, "  • Total in current map: %d\n", totalInMap);
        
    } else {
        fprintf(log, "  ℹ No NPCs loaded\n");
    }
    
    fprintf(log, "\n========================================\n");
    fprintf(log, "         DETAILED EXPLANATION\n");
    fprintf(log, "========================================\n");
    fprintf(log, "%s\n", details);
    
    fclose(log);
}

void SignalHandler(int signal)
{
    if (crashInProgress) exit(signal);
    crashInProgress = 1;
    
    // Print to console immediately with location details
    printf("\n\n*** CRASH DETECTED ***\n");
    printf("Signal: %d\n", signal);
    printf("World Pos: (%d,%d)\n", player.worldPos.x, player.worldPos.y);
    printf("Local Pos: (%d,%d)\n", player.localPos.x, player.localPos.y);
    printf("In Local Map: %s\n", player.isInLocalMap ? "Yes" : "No");
    printf("Map Size: %dx%d\n", currentMapWidth, currentMapHeight);
    printf("worldMap pointer: %p\n", (void*)worldMap);
    
    // Check current tile type
    if (worldMap && player.worldPos.y >= 0 && player.worldPos.y < currentMapHeight &&
        player.worldPos.x >= 0 && player.worldPos.x < currentMapWidth) {
        WorldTile* tile = &worldMap[player.worldPos.y][player.worldPos.x];
        printf("Current Tile: '%c'\n", tile->worldTile);
        printf("Location Type: %d - %s\n", tile->locationType, GetLocationTypeName(tile->locationType));
        printf("Location Name: %s\n", tile->locationName);
        printf("Has Local Map: %s\n", tile->hasLocalMap ? "Yes" : "No");
        if (tile->localMap) {
            printf("Local Map Exists: Yes\n");
            printf("Buildings: %d\n", tile->localMap->buildingCount);
            printf("Spawn Points: %d\n", tile->localMap->spawnPointCount);
        } else {
            printf("Local Map Exists: No\n");
        }
    }
    
    // Get detailed explanation
    const char* explanation = GetSignalExplanation(signal);
    
    // Save detailed crash log
    SaveDetailedCrashLog("Signal Crash", explanation, NULL);
    
    // Show dialog with explanation
    char fullMsg[2048];
    sprintf(fullMsg, 
        "%s\n\n"
        "A detailed crash log has been saved to 'crashlog.txt'\n\n"
        "Please report this crash to the developer with:\n"
        "- The crashlog.txt file\n"
        "- What you were doing when it crashed\n\n"
        "Press OK to exit.",
        explanation);
    
    ShowErrorBox(fullMsg, "BoneBound Has Crashed");
    
    exit(signal);
}

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* ep)
{
    if (crashInProgress) return EXCEPTION_EXECUTE_HANDLER;
    crashInProgress = 1;
    
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    const char* explanation = GetExceptionExplanation(code, ep);
    
    // Save detailed crash log
    SaveDetailedCrashLog("Exception Crash", explanation, ep);
    
    // Show dialog with explanation
    char fullMsg[2048];
    sprintf(fullMsg, 
        "%s\n\n"
        "A detailed crash log has been saved to 'crashlog.txt'\n\n"
        "Please report this crash to the developer with:\n"
        "- The crashlog.txt file\n"
        "- What you were doing when it crashed\n\n"
        "Press OK to exit.",
        explanation);
    
    ShowErrorBox(fullMsg, "BoneBound Has Crashed");
    
    return EXCEPTION_EXECUTE_HANDLER;
}

void SetupCrashHandler()
{
    // Set signal handlers
    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGFPE, SignalHandler);
    signal(SIGILL, SignalHandler);
    
    // Set Windows exception handler
    SetUnhandledExceptionFilter(ExceptionHandler);
    
    printf("Crash handler installed - crashes will be logged to crashlog.txt\n");
}

void CheckMemoryUsage()
{
    PROCESS_MEMORY_COUNTERS pmc;
    pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
    
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        SIZE_T memoryMB = pmc.WorkingSetSize / (1024 * 1024);
        
        if (memoryMB > 800) {
            char warningMsg[512];
            sprintf(warningMsg, 
                "⚠ MEMORY WARNING ⚠\n\n"
                "Memory usage is very high: %zu MB\n\n"
                "This can cause crashes. Recommended actions:\n"
                "• Save your game immediately\n"
                "• Restart the game\n"
                "• Avoid visiting too many areas without restarting\n\n"
                "Press OK to continue (game may become unstable).",
                memoryMB);
            
            ShowWarningBox(warningMsg, "Memory Warning");
        } else if (memoryMB > 600) {
            printf("Memory usage: %zu MB - Getting high\n", memoryMB);
        } else {
            printf("Memory usage: %zu MB\n", memoryMB);
        }
    }
}