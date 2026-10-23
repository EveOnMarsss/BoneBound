#ifndef HUD_H
#define HUD_H

#include "../project.h"

void DrawHUD();
const char* LocationTypeName(LocationType t);   // "Village", "Castle"... or NULL for plain wilderness
const char* TemperatureWord(float t);           // "cold", "mild", "hot"...

#endif
