#include "Tiles.h"

static TileDef g_defs[256];
static bool    g_defsReady = false;

unsigned int HashCell(int x, int y, unsigned int seed)
{
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return h;
}

static void Def(unsigned char code, unsigned char g0, unsigned char g1, unsigned char g2, unsigned char g3,
                Color c, bool walk, bool water, bool build, TileKind kind, const char* name)
{
    TileDef& d = g_defs[code];
    d.glyph[0] = g0; d.glyph[1] = g1; d.glyph[2] = g2; d.glyph[3] = g3;
    d.color = c; d.walkable = walk; d.water = water; d.buildable = build; d.kind = kind; d.name = name;
}

static void InitTileDefs()
{
    if (g_defsReady) return;
    memset(g_defs, 0, sizeof(g_defs));
    // default for unknown chars: shows '?', blocks movement (safe)
    for (int i = 0; i < 256; i++) {
        g_defs[i].glyph[0] = g_defs[i].glyph[1] = g_defs[i].glyph[2] = g_defs[i].glyph[3] = '?';
        g_defs[i].color = RAYWHITE; g_defs[i].name = "unknown";
    }

    // ---- natural terrain ----        glyphs (CP437)         color                 walk   water  build  kind
    Def('.', 250, 250,  39,  44, (Color){ 100, 255, 100, 255 }, true,  false, true,  TILEKIND_GROUND, "grass");
    Def('"',  34,  34,  39,  34, (Color){ 110, 235,  90, 255 }, true,  false, true,  TILEKIND_GROUND, "tall grass");
    Def('*',  42,  42,  42,  15, (Color){ 255, 160, 200, 255 }, true,  false, true,  TILEKIND_GROUND, "wildflowers");
    Def(':', 176, 176, 250, 176, (Color){ 225, 205, 130, 255 }, true,  false, true,  TILEKIND_GROUND, "sand");
    Def('N', 249, 250, 249, 250, (Color){ 235, 240, 250, 255 }, true,  false, true,  TILEKIND_GROUND, "snow");
    Def('m', 177, 176, 247, 177, (Color){ 105,  85,  55, 255 }, true,  false, true,  TILEKIND_GROUND, "mud");
    Def('w', 247, 126, 247, 247, (Color){  90, 150, 240, 255 }, true,  true,  false, TILEKIND_WATER,  "shallow water");
    Def('~', 247, 247, 126, 247, (Color){  40,  85, 190, 255 }, false, true,  false, TILEKIND_WATER,  "deep water");
    Def('t',   6,   5,   6,   5, (Color){  40, 125,  40, 255 }, false, false, true,  TILEKIND_PLANT,  "tree");
    Def('Y',  24,  24,  24,  24, (Color){  40, 105,  80, 255 }, false, false, true,  TILEKIND_PLANT,  "pine");
    Def('c', 159, 159, 159, 159, (Color){  90, 160,  75, 255 }, false, false, true,  TILEKIND_PLANT,  "cactus");
    Def('h', 110, 110, 110, 239, (Color){ 150, 130,  85, 255 }, true,  false, true,  TILEKIND_GROUND, "hillside");
    Def('^',  30,  30,  30,  30, (Color){ 135, 130, 125, 255 }, false, false, false, TILEKIND_ROCK,   "rock face");
    Def('o',   7,   7,   7,   7, (Color){ 140, 135, 130, 255 }, false, false, true,  TILEKIND_ROCK,   "boulder");
    Def('=', 205, 205, 205, 205, (Color){ 155, 110,  60, 255 }, true,  false, false, TILEKIND_STRUCT, "bridge");
    Def(',', 249, 250,  44, 249, (Color){ 150, 115,  70, 255 }, true,  false, true,  TILEKIND_GROUND, "dirt road");

    // ---- buildings & furniture (same chars as before; walkability now matches the old comments) ----
    Def('#',  35,  35,  35,  35, GRAY,                          false, false, false, TILEKIND_STRUCT, "wall");
    Def('D',  43,  43,  43,  43, YELLOW,                        true,  false, false, TILEKIND_STRUCT, "door");
    Def('B', 'B', 'B', 'B', 'B', BROWN,                         false, false, false, TILEKIND_STRUCT, "bed");
    Def('C', 'C', 'C', 'C', 'C', BEIGE,                         false, false, false, TILEKIND_STRUCT, "chair");
    Def('T', 209, 209, 209, 209, (Color){ 170, 120,  70, 255 }, false, false, false, TILEKIND_STRUCT, "table");
    Def('S', 'S', 'S', 'S', 'S', LIGHTGRAY,                     false, false, false, TILEKIND_STRUCT, "shelf");
    Def('A', 'A', 'A', 'A', 'A', DARKGRAY,                      false, false, false, TILEKIND_STRUCT, "anvil");
    Def('F',  15,  15,  15,  15, ORANGE,                        false, false, false, TILEKIND_STRUCT, "fountain/fireplace");
    Def('H', 'H', 'H', 'H', 'H', GOLD,                          false, false, false, TILEKIND_STRUCT, "chest");
    Def('P', 'P', 'P', 'P', 'P', PURPLE,                        false, false, false, TILEKIND_STRUCT, "pillar");
    Def('R', 'R', 'R', 'R', 'R', DARKPURPLE,                    false, false, false, TILEKIND_STRUCT, "altar");
    Def('K', 'K', 'K', 'K', 'K', RED,                           false, false, false, TILEKIND_STRUCT, "throne");
    Def('<',  31,  31,  31,  31, (Color){ 255, 255, 140, 255 }, true,  false, false, TILEKIND_STRUCT, "stairs down");
    Def('>',  30,  30,  30,  30, (Color){ 255, 255, 140, 255 }, true,  false, false, TILEKIND_STRUCT, "stairs up");
    Def('Q', 'Q', 'Q', 'Q', 'Q', BROWN,                         false, false, false, TILEKIND_STRUCT, "barrel");

    g_defsReady = true;
}

const TileDef& GetTileDef(char tile)
{
    InitTileDefs();
    return g_defs[(unsigned char)tile];
}

bool TileWalkable(char tile)  { return GetTileDef(tile).walkable; }
bool TileIsWater(char tile)   { return GetTileDef(tile).water; }
bool TileBuildable(char tile) { return GetTileDef(tile).buildable; }
const char* TileName(char tile) { return GetTileDef(tile).name; }

Color GetTileColor(char tile) { return GetTileDef(tile).color; }

int TileGlyph(char tile, unsigned int h)
{
    const TileDef& d = GetTileDef(tile);
    return d.glyph[h & 3];
}

// ---- biome tints -----------------------------------------------------------
static Color GrassTint(int biome)
{
    switch (biome) {
        case BIOME_DESERT:     return (Color){ 175, 155,  85, 255 };
        case BIOME_SAVANNA:    return (Color){ 175, 170,  75, 255 };
        case BIOME_GRASSLAND:  return (Color){  95, 175,  65, 255 };
        case BIOME_FOREST:     return (Color){  65, 145,  55, 255 };
        case BIOME_JUNGLE:     return (Color){  40, 165,  65, 255 };
        case BIOME_SWAMP:      return (Color){  95, 125,  65, 255 };
        case BIOME_TAIGA:      return (Color){  85, 130, 100, 255 };
        case BIOME_TUNDRA:     return (Color){ 135, 155, 115, 255 };
        case BIOME_HILLS:      return (Color){ 120, 150,  75, 255 };
        case BIOME_MOUNTAINS:  return (Color){ 120, 135,  95, 255 };
        case BIOME_SNOW_PEAKS: return (Color){ 160, 175, 160, 255 };
        case BIOME_GLACIER:    return (Color){ 160, 190, 170, 255 };
        case BIOME_BEACH:      return (Color){ 140, 165,  85, 255 };
        default:               return (Color){ 100, 190,  70, 255 };
    }
}

static Color TreeTint(int biome)
{
    switch (biome) {
        case BIOME_JUNGLE:   return (Color){  20, 135,  45, 255 };
        case BIOME_SAVANNA:  return (Color){ 150, 140,  55, 255 };
        case BIOME_TAIGA:    return (Color){  40, 110,  85, 255 };
        case BIOME_TUNDRA:   return (Color){  95, 120,  85, 255 };
        case BIOME_SWAMP:    return (Color){  65, 100,  55, 255 };
        case BIOME_DESERT:   return (Color){ 100, 150,  70, 255 };
        case BIOME_HILLS:    return (Color){  55, 125,  50, 255 };
        default:             return (Color){  40, 130,  40, 255 };
    }
}

static unsigned char ClampByte(int v) { return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v)); }

static Color Jitter(Color c, unsigned int h, int amount)
{
    int b = (int)((h >> 8) & 0xFF);                    // 0..255
    int f = ((b * amount) / 255) - amount / 2;         // -amount/2 .. +amount/2 (percent)
    int dr = (int)((h >> 16) & 7) - 3;
    int dg = (int)((h >> 19) & 7) - 3;
    int db = (int)((h >> 22) & 7) - 3;
    c.r = ClampByte(c.r + c.r * f / 100 + dr);
    c.g = ClampByte(c.g + c.g * f / 100 + dg);
    c.b = ClampByte(c.b + c.b * f / 100 + db);
    return c;
}

Color TileColor(char tile, int biome, unsigned int h)
{
    const TileDef& d = GetTileDef(tile);
    Color c = d.color;

    switch (tile) {
        case TILE_GRASS:     c = GrassTint(biome); break;
        case TILE_TALLGRASS: { Color g = GrassTint(biome); c = (Color){ ClampByte(g.r + 15), ClampByte(g.g + 20), ClampByte(g.b - 5), 255 }; break; }
        case TILE_TREE:      c = TreeTint(biome); break;
        case TILE_PINE:      c = TreeTint(biome == BIOME_TAIGA ? biome : BIOME_TAIGA); break;
        case TILE_HILL:      { Color g = GrassTint(biome); c = (Color){ (unsigned char)((g.r + d.color.r) / 2), (unsigned char)((g.g + d.color.g) / 2), (unsigned char)((g.b + d.color.b) / 2), 255 }; break; }
        case TILE_ROCK:      if (biome == BIOME_SNOW_PEAKS) c = (Color){ 240, 245, 255, 255 };
                             else if (biome == BIOME_GLACIER) c = (Color){ 200, 225, 245, 255 };
                             break;
        case TILE_SAND:      if (biome == BIOME_DESERT) c = (Color){ 235, 200, 120, 255 }; break;
        case TILE_FLOWERS: {
            static const Color pal[5] = { {255,150,200,255}, {255,230,90,255}, {245,245,255,255}, {190,140,255,255}, {255,120,90,255} };
            c = pal[(h >> 5) % 5];
            return c;   // flowers keep their pop, no jitter
        }
        default: break;
    }

    switch (d.kind) {
        case TILEKIND_STRUCT: return c;                     // buildings stay crisp
        case TILEKIND_WATER:  return Jitter(c, h, 14);
        default:              return Jitter(c, h, 24);
    }
}

Color TileBackground(char tile, int biome, unsigned int h)
{
    const TileDef& d = GetTileDef(tile);
    (void)h;
    switch (d.kind) {
        case TILEKIND_WATER:
            return tile == TILE_DEEP ? (Color){ 6, 16, 52, 255 } : (Color){ 12, 38, 92, 255 };
        case TILEKIND_PLANT: {
            Color g = GrassTint(biome);
            return (Color){ (unsigned char)(g.r / 7), (unsigned char)(g.g / 7), (unsigned char)(g.b / 7), 255 };
        }
        case TILEKIND_GROUND: {
            Color c = TileColor(tile, biome, 0);
            if (tile == TILE_FLOWERS) c = GrassTint(biome);
            return (Color){ (unsigned char)(c.r / 8), (unsigned char)(c.g / 8), (unsigned char)(c.b / 8), 255 };
        }
        case TILEKIND_ROCK: {
            Color c = TileColor(tile, biome, 0);
            return (Color){ (unsigned char)(c.r / 9), (unsigned char)(c.g / 9), (unsigned char)(c.b / 9), 255 };
        }
        default:
            return (Color){ 0, 0, 0, 0 };
    }
}
