#ifndef NAME_GEN_H
#define NAME_GEN_H

#include <string>
#include "../Systems/DataLoader.h"   // gives you Culture and NameParts, so we do not redefine them
#include "WorldRng.h"               
// Builds a name from a culture's word pieces, like "Barkville" (forPerson = false) or "Snufins" (forPerson = true).
std::string MakeName(const Culture& culture, bool forPerson, Rng& rng);

// Which culture does the world tile (tileX, tileY) belong to? Returns nullptr when no cultures are loaded.
const Culture* CultureForTile(int tileX, int tileY, int worldWidth);

#endif // NAME_GEN_H