#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>

// FIX: the old version declared `static FILE* logFile` in this header, so every .cpp file got
// its own copy and each one reopened debug_log.txt with "w" - they kept truncating each other.
// Now there is exactly ONE log file handle, defined in Globals.cpp.
extern FILE* logFile;
void InitLogger();
void CloseLogger();

#define LOG(...) \
    do { \
        InitLogger(); \
        if (logFile) { \
            fprintf(logFile, __VA_ARGS__); \
            fflush(logFile); \
        } \
    } while(0)

#define LOG_PRINTF(...) LOG(__VA_ARGS__)

#endif
