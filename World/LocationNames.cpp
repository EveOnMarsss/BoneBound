#include "LocationNames.h"
#include <ctype.h>
#include <stdio.h>

template <size_t N> static const char* Pick(const char* const (&arr)[N], Rng& rng) { return arr[rng.next() % N]; }

const char* BiomeName(BiomeType biome)
{
    switch (biome) {
        case BIOME_DEEP_OCEAN: return "Deep Ocean";
        case BIOME_OCEAN:      return "Ocean";
        case BIOME_LAKE:       return "Lake";
        case BIOME_BEACH:      return "Beach";
        case BIOME_DESERT:     return "Desert";
        case BIOME_SAVANNA:    return "Savanna";
        case BIOME_GRASSLAND:  return "Grasslands";
        case BIOME_FOREST:     return "Forest";
        case BIOME_JUNGLE:     return "Jungle";
        case BIOME_SWAMP:      return "Swamp";
        case BIOME_TAIGA:      return "Taiga";
        case BIOME_TUNDRA:     return "Tundra";
        case BIOME_GLACIER:    return "Glacier";
        case BIOME_HILLS:      return "Hills";
        case BIOME_MOUNTAINS:  return "Mountains";
        case BIOME_SNOW_PEAKS: return "Snowy Peaks";
        default:               return "Unknown";
    }
}

static const char* const VILLAGE_PRE[] = {
    "Oak", "River", "Stone", "Mill", "Fair", "Green", "Wolf", "East", "West", "North", "South", "Dust",
    "Fern", "Maple", "Willow", "Ash", "Briar", "Crest", "Dale", "Elm", "Amber", "Bright", "Cold", "Fox",
    "Hazel", "Iron", "Lark", "Moss", "Pine", "Raven", "Rush", "Sunny", "Thorn", "Wren", "Birch", "Clover",
    "Heron", "Marsh", "Rook", "Sage", "Bone", "Bark", "Whisker", "Paw", "Burrow", "Tail", "Fetch", "Purr"
};
static const char* const VILLAGE_SUF_PLAIN[]  = { "haven", "wood", "field", "den", "wick", "ton", "stead", "hollow", "glen", "moor", "gate", "cross", "well", "shire", "croft", "vale" };
static const char* const VILLAGE_SUF_COAST[]  = { "port", "mouth", "cove", "haven", "wick", "harbor", "bay", "strand" };
static const char* const VILLAGE_SUF_RIVER[]  = { "ford", "bridge", "brook", "mill", "mere", "bend", "wash", "run" };
static const char* const VILLAGE_SUF_HILL[]   = { "ridge", "hill", "crag", "peak", "tor", "cliff", "fell", "knoll" };
static const char* const VILLAGE_SUF_FOREST[] = { "wood", "grove", "thicket", "hollow", "glade", "weald", "copse", "shaw" };

static const char* const CASTLE_PRE[] = { "Iron", "Storm", "Raven", "Wolf", "Crow", "Black", "Grey", "High", "Dread", "Silver", "Gold", "Thorn", "Ember", "Frost", "Hollow", "Stone", "Lion", "Eagle", "Moon", "Sun" };
static const char* const CASTLE_SUF[] = { "keep", "hold", "spire", "crag", "fort", "rock", "watch", "hall", "reach", "guard", "crest", "bastion" };

static const char* const CAVE_PRE[] = { "Gloom", "Echo", "Dark", "Crystal", "Shadow", "Whisper", "Fungal", "Goblin", "Troll", "Bat", "Lost", "Deep", "Obsidian", "Moss", "Drip" };
static const char* const CAVE_SUF[] = { "deep", "chasm", "maw", "grotto", "den", "cavern", "hollow", "burrow", "warren", "pit" };

static const char* const TOWER_PRE[] = { "Star", "Moon", "Arcane", "Mystic", "Crystal", "Spell", "Rune", "Dusk", "Dawn", "Whisper", "Ember", "Mist", "Void", "Sage" };
static const char* const TOWER_SUF[] = { "spire", "tower", "needle", "keep", "rise", "observatory", "sanctum", "pinnacle" };

static const char* const DUNGEON_PRE[] = { "Soul", "Agony", "Barrow", "Crypt", "Despair", "Forsaken", "Grief", "Horror", "Infernal", "Sorrow", "Rot", "Shade", "Gallows", "Bleak" };
static const char* const DUNGEON_SUF[] = { "pit", "deep", "prison", "vault", "gate", "depths", "keep", "catacombs", "maze", "hold" };

static const char* const RUIN_ADJ[] = { "Fallen", "Lost", "Ancient", "Crumbled", "Forgotten", "Ghostly", "Haunted", "Sunken", "Silent", "Broken", "Weathered", "Overgrown" };
static const char* const RUIN_NOUN[] = { "Kingdom", "City", "Citadel", "Spire", "Village", "Abbey", "Gardens", "Bridge", "Halls", "Outpost", "Watchtower", "Market" };

static const char* const FORT_PRE[] = { "Stone", "Iron", "Steel", "Battle", "Warren", "Shield", "Garrison", "Hammer", "Bulwark", "Vanguard", "Border", "Pass", "Sentinel", "Ward" };
static const char* const FORT_SUF[] = { "guard", "wall", "keep", "ford", "hold", "fall", "watch", "rest", "post", "gate" };

static const char* const TEMPLE_OF[] = { "the Sun", "the Moon", "the Old Ones", "Still Waters", "the Wild Hunt", "Gentle Rain", "the First Bone", "Nine Lives", "the Long Road", "the Hearth", "Eternal Dawn", "the Silent Stars" };

void GenerateLocationName(char* out, int size, LocationType type, NameFlavor flavor, Rng& rng)
{
    switch (type) {
        case LOCATION_VILLAGE: {
            const char* suf;
            switch (flavor) {
                case NAME_COAST:  suf = Pick(VILLAGE_SUF_COAST, rng); break;
                case NAME_RIVER:  suf = Pick(VILLAGE_SUF_RIVER, rng); break;
                case NAME_HILL:   suf = Pick(VILLAGE_SUF_HILL, rng); break;
                case NAME_FOREST: suf = Pick(VILLAGE_SUF_FOREST, rng); break;
                default:          suf = Pick(VILLAGE_SUF_PLAIN, rng); break;
            }
            snprintf(out, size, "%s%s", Pick(VILLAGE_PRE, rng), suf);
            break;
        }
        case LOCATION_CASTLE:       snprintf(out, size, "%s%s", Pick(CASTLE_PRE, rng), Pick(CASTLE_SUF, rng)); break;
        case LOCATION_CAVE:         snprintf(out, size, "%s%s", Pick(CAVE_PRE, rng), Pick(CAVE_SUF, rng)); break;
        case LOCATION_WIZARD_TOWER: snprintf(out, size, "%s%s", Pick(TOWER_PRE, rng), Pick(TOWER_SUF, rng)); break;
        case LOCATION_DUNGEON:      snprintf(out, size, "%s%s", Pick(DUNGEON_PRE, rng), Pick(DUNGEON_SUF, rng)); break;
        case LOCATION_RUINS:        snprintf(out, size, "%s %s", Pick(RUIN_ADJ, rng), Pick(RUIN_NOUN, rng)); break;
        case LOCATION_FORT:         snprintf(out, size, "%s%s", Pick(FORT_PRE, rng), Pick(FORT_SUF, rng)); break;
        case LOCATION_TEMPLE:       snprintf(out, size, "Temple of %s", Pick(TEMPLE_OF, rng)); break;
        default:                    snprintf(out, size, "Unknown"); break;
    }
    if (size > 0) out[0] = (char)toupper((unsigned char)out[0]);
}
