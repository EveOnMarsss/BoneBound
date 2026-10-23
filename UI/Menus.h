#ifndef MENUS_H
#define MENUS_H

#include "../project.h"

void TitleUpdate();
void TitleDraw();
void MapSizeUpdate();
void MapSizeDraw();
void SaveMenuUpdate();
void SaveMenuDraw();
void LoadMenuUpdate();
void LoadMenuDraw();
void CharacterCreationUpdate();
void CharacterCreationDraw();
void PauseUpdate();
void PauseDraw();
void GeneratingUpdate();
void GeneratingDraw();
void StartPickUpdate();
void StartPickDraw();

extern GameState menuReturnState;   // where Save/Load menus go back to

#endif
