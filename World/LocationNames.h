#ifndef LOCATION_NAMES_H
#define LOCATION_NAMES_H

#include "../project.h"
#include "WorldRng.h"

// What the surroundings look like - influences the name ("Fox" + "mouth" on a coast, "ford" on a river...)
typedef enum { NAME_PLAIN = 0, NAME_COAST, NAME_RIVER, NAME_HILL, NAME_FOREST } NameFlavor;

void GenerateLocationName(char* out, int size, LocationType type, NameFlavor flavor, Rng& rng);
const char* BiomeName(BiomeType biome);

#endif
