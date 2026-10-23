#ifndef SPAWN_POINT_H
#define SPAWN_POINT_H

#include "../project.h"

void CreateTownSpawnPoints(LocalMap* map);
void CreateBanditCampSpawnPoints(LocalMap* map);
void CreateCastleSpawnPoints(LocalMap* map);
void CreateCaveSpawnPoints(LocalMap* map);
void CreateTowerSpawnPoints(LocalMap* map);
void CreateDungeonSpawnPoints(LocalMap* map);
void CreateRuinsSpawnPoints(LocalMap* map);
void CreateFortSpawnPoints(LocalMap* map);
void SpawnNPCsFromPoints(LocalMap* map, int worldX, int worldY);
void UpdateSpawnPoints();
void NPCDied(NPC* npc);

#endif