#ifndef NPC_H
#define NPC_H

#include "../project.h"

// Name generator - make sure these match the definitions in NPC.cpp
extern const char* FIRST_NAMES[];
extern const char* LAST_NAMES[];
extern const int FIRST_NAME_COUNT;
extern const int LAST_NAME_COUNT;

NPC* CreateNPCFromSpawn(SpawnPoint* spawn, int worldX, int worldY);
NPC* GetNPCAt(int worldX, int worldY, int localX, int localY);   // living NPC standing on that cell, or NULL
void UpdateNPCs();
void DrawNPCs();
void InteractWithNPC();
void NPCTakeDamage(NPC* npc, int damage);
void GenerateNPCName(char* buffer, int bufferSize, bool isUnique);

#endif