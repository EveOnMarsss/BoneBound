#ifndef SAVE_LOAD_H
#define SAVE_LOAD_H

#include "../project.h"

// Summary shown in the save/load menus
typedef struct {
    char name[32];
    bool isCat;
    int level;
    char locationName[50];
    int worldWidth, worldHeight;
    unsigned int seed;
} SaveInfo;

bool SaveFileExists(int slot);
bool SaveGame(int slot);                          // true on success
bool LoadGame(int slot);                          // false if missing, damaged or from an incompatible version
bool ReadSaveInfo(int slot, SaveInfo* info);      // false if the file can't be read / wrong version

#endif
