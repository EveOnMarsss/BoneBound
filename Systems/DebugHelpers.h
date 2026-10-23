#ifndef DEBUG_HELPERS_H
#define DEBUG_HELPERS_H

#include "../project.h"
#include <windows.h>

#ifdef _DEBUG
    #define ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                char buffer[512]; \
                sprintf(buffer, "Assertion Failed: %s\nFile: %s\nLine: %d\n%s", \
                        #condition, __FILE__, __LINE__, message); \
                MessageBoxA(NULL, buffer, "Debug Assertion", MB_OK | MB_ICONERROR); \
                DebugBreak(); \
            } \
        } while(0)
#else
    #define ASSERT(condition, message) ((void)0)
#endif

// Safe position checks (always on)
inline void AssertValidWorldPosition(int x, int y)
{
    if (x < 0 || x >= currentMapWidth || y < 0 || y >= currentMapHeight) {
        char msg[256];
        sprintf(msg, "CRITICAL: Invalid world position (%d,%d)!\nMap size: %dx%d\nGame will now crash.", 
                x, y, currentMapWidth, currentMapHeight);
        MessageBoxA(NULL, msg, "Fatal Position Error", MB_OK | MB_ICONERROR);
    }
}

inline void AssertValidLocalPosition(int x, int y)
{
    if (x < 0 || x >= LOCAL_MAP_WIDTH || y < 0 || y >= LOCAL_MAP_HEIGHT) {
        char msg[256];
        sprintf(msg, "CRITICAL: Invalid local position (%d,%d)!\nMap size: %dx%d\nGame will now crash.", 
                x, y, LOCAL_MAP_WIDTH, LOCAL_MAP_HEIGHT);
        MessageBoxA(NULL, msg, "Fatal Position Error", MB_OK | MB_ICONERROR);
    }
}

// This will trigger a message box if localPos is accessed when not in local map
#define ASSERT_LOCAL_MAP_ACCESS() \
    do { \
        if (!player.isInLocalMap) { \
            char msg[256]; \
            sprintf(msg, "CRITICAL: localPos accessed while not in local map!\n" \
                         "File: %s\nLine: %d\n" \
                         "localPos=(%d,%d)", \
                         __FILE__, __LINE__, \
                         player.localPos.x, player.localPos.y); \
            MessageBoxA(NULL, msg, "Local Map Access Error", MB_OK | MB_ICONERROR); \
            DebugBreak(); \
        } \
    } while(0)

// Call this before any localPos access
#define SAFE_LOCAL_POS(var) \
    (ASSERT_LOCAL_MAP_ACCESS(), var)

#endif