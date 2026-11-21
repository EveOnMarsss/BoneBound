#ifndef NPC_H
#define NPC_H

#include "../project.h"

// Name generator - make sure these match the definitions in NPC.cpp

NPC* CreateNPCFromSpawn(SpawnPoint* spawn, int worldX, int worldY);
NPC* GetNPCAt(int worldX, int worldY, int localX, int localY);   // living NPC standing on that cell, or NULL
void UpdateNPCs();
void DrawNPCs();
void InteractWithNPC();
void NPCTakeDamage(NPC* npc, int damage);

#endif