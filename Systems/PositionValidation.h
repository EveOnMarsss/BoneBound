#ifndef POSITION_VALIDATION_H
#define POSITION_VALIDATION_H

#include "../project.h"

// Call this BEFORE any function that uses localPos
#define REQUIRE_LOCAL_MAP() \
    do { \
        if (!player.isInLocalMap) { \
            printf("FATAL: Tried to use localPos while not in local map at %s:%d\n", \
                   __FILE__, __LINE__); \
            /* Write to crash log */ \
            FILE* log = fopen("crashlog.txt", "a"); \
            if (log) { \
                fprintf(log, "ILLEGAL LOCAL POS ACCESS at %s:%d\n", __FILE__, __LINE__); \
                fclose(log); \
            } \
            /* Force crash with clear message */ \
            char msg[256]; \
            sprintf(msg, "localPos used while not in local map!\nFile: %s\nLine: %d", \
                    __FILE__, __LINE__); \
            MessageBoxA(NULL, msg, "Fatal Error", MB_OK | MB_ICONERROR); \
            exit(1); \
        } \
    } while(0)

// Call this BEFORE any function that uses worldPos
#define REQUIRE_WORLD_MAP() \
    do { \
        if (player.isInLocalMap) { \
            printf("FATAL: Tried to use worldPos while in local map at %s:%d\n", \
                   __FILE__, __LINE__); \
            FILE* log = fopen("crashlog.txt", "a"); \
            if (log) { \
                fprintf(log, "ILLEGAL WORLD POS ACCESS at %s:%d\n", __FILE__, __LINE__); \
                fclose(log); \
            } \
            char msg[256]; \
            sprintf(msg, "worldPos used while in local map!\nFile: %s\nLine: %d", \
                    __FILE__, __LINE__); \
            MessageBoxA(NULL, msg, "Fatal Error", MB_OK | MB_ICONERROR); \
            exit(1); \
        } \
    } while(0)

#endif