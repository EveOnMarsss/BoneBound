#include "LocalTerrain.h"
#include "Tiles.h"
#include "WorldGen.h"
#include "FastNoiseLite.h"
#include <math.h>
#include <string.h>
#include <algorithm>
#include <vector>

namespace {

inline float Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
inline float Smooth(float a, float b, float x) { float t = Clamp01((x - a) / (b - a)); return t * t * (3.f - 2.f * t); }
inline float Mix(float a, float b, float t) { return a + (b - a) * t; }
inline float Hash01(int x, int y, unsigned int seed) { return (float)((HashCell(x, y, seed) >> 8) & 0xFFFF) / 65536.f; }

struct LNoise {
    unsigned int seed;
    FastNoiseLite hills, patch, detail, moist, ridge, wobble, trail;
    explicit LNoise(unsigned int s) : seed(s)
    {
        int si = (int)s;
        hills.SetSeed(si + 11);  hills.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        hills.SetFractalType(FastNoiseLite::FractalType_FBm); hills.SetFractalOctaves(3); hills.SetFrequency(1.f);
        patch.SetSeed(si + 22);  patch.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        patch.SetFractalType(FastNoiseLite::FractalType_FBm); patch.SetFractalOctaves(3); patch.SetFrequency(1.f);
        detail.SetSeed(si + 33); detail.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        detail.SetFractalType(FastNoiseLite::FractalType_FBm); detail.SetFractalOctaves(2); detail.SetFrequency(1.f);
        moist.SetSeed(si + 44);  moist.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        moist.SetFractalType(FastNoiseLite::FractalType_FBm); moist.SetFractalOctaves(3); moist.SetFrequency(1.f);
        ridge.SetSeed(si + 55);  ridge.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        ridge.SetFractalType(FastNoiseLite::FractalType_Ridged); ridge.SetFractalOctaves(3); ridge.SetFrequency(1.f);
        trail.SetSeed(si + 77);  trail.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); trail.SetFrequency(1.f);
        wobble.SetSeed(si + 66); wobble.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); wobble.SetFrequency(1.f);
    }
};

LNoise* g_noise = NULL;
LNoise& Noise(unsigned int seed)
{
    if (!g_noise || g_noise->seed != seed) { delete g_noise; g_noise = new LNoise(seed); }
    return *g_noise;
}

struct Field { float e, t, r; };

Field TileField(int wx, int wy)
{
    bool oob = (wx < 0 || wy < 0 || wx >= currentMapWidth || wy >= currentMapHeight);
    int cx = wx < 0 ? 0 : (wx >= currentMapWidth ? currentMapWidth - 1 : wx);
    int cy = wy < 0 ? 0 : (wy >= currentMapHeight ? currentMapHeight - 1 : wy);
    const WorldTile& t = worldMap[cy][cx];
    Field f = { t.elevation, t.temperature, t.moisture };
    if (oob) f.e = std::min(f.e, 0.12f);          // beyond the world edge is open ocean
    return f;
}

inline Field Bilerp(const Field& a, const Field& b, const Field& c, const Field& d, float fx, float fy)
{
    Field r;
    r.e = Mix(Mix(a.e, b.e, fx), Mix(c.e, d.e, fx), fy);
    r.t = Mix(Mix(a.t, b.t, fx), Mix(c.t, d.t, fx), fy);
    r.r = Mix(Mix(a.r, b.r, fx), Mix(c.r, d.r, fx), fy);
    return r;
}

float SettlementRadius(LocationType t)
{
    switch (t) {
        case LOCATION_VILLAGE: return 118.f;
        case LOCATION_CASTLE:  return 100.f;
        case LOCATION_FORT:    return 75.f;
        case LOCATION_TEMPLE:  return 70.f;
        case LOCATION_WIZARD_TOWER: return 60.f;
        case LOCATION_RUINS:   return 85.f;
        case LOCATION_CAVE:    return 62.f;
        case LOCATION_DUNGEON: return 55.f;
        default: return 0.f;
    }
}

// ---------------------------------------------------------------------------------------------
// Ground cover: pick the tile for one cell
// ---------------------------------------------------------------------------------------------
struct Ctx {
    float elevL, tempL, rainL;
    float nA, nC, ridge;     // patch noise, detail noise, ridge noise at this cell
    float trail;             // |trail| small => a natural game trail / pass runs through this cell
    float r1, r2;            // per-cell random numbers
    float clearing;          // 0..1, how strongly obstacles are suppressed (settlements, roads)
};

char PickLand(BiomeType b, const Ctx& c)
{
    // natural trails: continuous winding corridors with no trees/rocks, so dense forests and mountains stay passable
    const bool onTrail = fabsf(c.trail) < 0.030f;
    const float keep = onTrail ? 0.f : 1.f - Clamp01(c.clearing * 1.15f);   // multiplier for obstacle density
    const float shore = c.elevL - SEA_LEVEL;

    // shorelines: sand (mud in swamps); cliffs and glaciers go straight to the water
    if (shore < 0.012f + c.nC * 0.006f && b != BIOME_MOUNTAINS && b != BIOME_SNOW_PEAKS && b != BIOME_GLACIER)
        return (b == BIOME_SWAMP) ? TILE_MUD : TILE_SAND;

    switch (b) {
        case BIOME_GRASSLAND: {
            float treeP = (c.nA > 0.45f) ? 0.28f : (c.nA > 0.25f ? 0.05f : 0.004f);
            if (c.r1 < treeP * keep) return TILE_TREE;
            if (c.r2 > 0.9985f && keep > 0.5f) return TILE_BOULDER;
            if (c.r2 < 0.014f && c.nA > -0.3f) return TILE_FLOWERS;
            return (c.nC > 0.15f) ? TILE_TALLGRASS : TILE_GRASS;
        }
        case BIOME_FOREST: {
            float dens = 0.22f + 0.28f * c.rainL + 0.30f * c.nA;
            if (c.nA < -0.45f) dens *= 0.15f;                       // meadows
            if (c.r1 < dens * keep) return (c.tempL < 0.5f && c.r2 < 0.35f) ? TILE_PINE : TILE_TREE;
            if (c.r2 < 0.012f) return TILE_FLOWERS;
            return (c.nC > 0.25f) ? TILE_TALLGRASS : TILE_GRASS;
        }
        case BIOME_JUNGLE: {
            float dens = 0.42f + 0.22f * c.nA;
            if (c.r1 < dens * keep) return TILE_TREE;
            return (c.r2 < 0.7f) ? TILE_TALLGRASS : TILE_GRASS;
        }
        case BIOME_SAVANNA: {
            float treeP = (c.nA > 0.55f) ? 0.12f : 0.005f;
            if (c.r1 < treeP * keep) return TILE_TREE;
            if (c.r2 > 0.9988f && keep > 0.5f) return TILE_BOULDER;
            return (c.r2 < 0.6f) ? TILE_TALLGRASS : TILE_GRASS;
        }
        case BIOME_TAIGA: {
            float dens = 0.18f + 0.26f * c.rainL + 0.26f * c.nA;
            if (c.r1 < dens * keep) return TILE_PINE;
            if (c.tempL < 0.34f && c.nC > 0.20f) return TILE_SNOW;
            return TILE_GRASS;
        }
        case BIOME_TUNDRA: {
            if (c.nC + (0.32f - c.tempL) * 1.6f > 0.25f) return TILE_SNOW;
            if (c.r2 > 0.99f && keep > 0.5f) return TILE_BOULDER;
            if (c.r1 < 0.004f * keep) return TILE_PINE;
            return TILE_GRASS;
        }
        case BIOME_GLACIER:
            if (c.r2 > 0.997f && keep > 0.5f) return TILE_BOULDER;
            return TILE_SNOW;
        case BIOME_DESERT: {
            if (c.r2 < 0.006f * keep) return TILE_CACTUS;
            if (c.r2 > 0.994f && keep > 0.5f) return TILE_BOULDER;
            if (c.nC > 0.45f && c.r1 < 0.30f) return TILE_TALLGRASS;      // dry scrub
            return TILE_SAND;
        }
        case BIOME_SWAMP: {
            if (c.nC < -0.30f) return TILE_SHALLOW;                      // pools
            if (c.r1 < 0.10f * keep) return TILE_TREE;
            if (c.nA > -0.15f) return TILE_MUD;
            return (c.r2 < 0.5f) ? TILE_TALLGRASS : TILE_GRASS;
        }
        case BIOME_HILLS: {
            float treeP = (c.nA > 0.30f) ? 0.12f : 0.03f;
            if (c.r1 < treeP * keep) return TILE_TREE;
            if (c.r2 > 0.985f && keep > 0.5f) return TILE_BOULDER;
            if (c.r2 < 0.6f) return TILE_HILL;
            return (c.nC > 0.1f) ? TILE_TALLGRASS : TILE_GRASS;
        }
        case BIOME_MOUNTAINS: {
            float thr = 0.34f - (c.elevL - 0.70f) * 1.3f;
            if (c.ridge > thr && keep > 0.4f) return TILE_ROCK;
            if (c.r2 > 0.93f && keep > 0.5f) return TILE_BOULDER;
            if (c.tempL < 0.30f && c.nC > 0.1f) return TILE_SNOW;
            return (c.r1 < 0.6f) ? TILE_HILL : TILE_GRASS;
        }
        case BIOME_SNOW_PEAKS: {
            float thr = 0.18f - (c.elevL - 0.84f) * 1.3f;
            if (c.ridge > thr && keep > 0.4f) return TILE_ROCK;
            if (c.r2 > 0.97f && keep > 0.5f) return TILE_BOULDER;
            return TILE_SNOW;
        }
        default:
            return TILE_GRASS;
    }
}

// ---------------------------------------------------------------------------------------------
// Rivers and roads: polylines from a tile's centre node to the border points shared with neighbours
// ---------------------------------------------------------------------------------------------
struct Pt { float x, y; };

Pt EdgePoint(int wx, int wy, int d, unsigned int seed, unsigned int salt)
{
    const float M = (float)(LOCAL_MAP_WIDTH - 1);
    switch (d) {
        case 1: return { M, 0.f };
        case 3: return { M, M };
        case 5: return { 0.f, M };
        case 7: return { 0.f, 0.f };
        default: break;
    }
    // cardinal edge shared by two tiles: key on the canonical owner so both sides agree on the crossing point
    int kx = wx, ky = wy, axis = 0;                          // axis 0: vertical edge (E/W), 1: horizontal edge (N/S)
    if (d == 2) { axis = 0; }
    else if (d == 6) { kx = wx - 1; axis = 0; }
    else if (d == 4) { axis = 1; }
    else { ky = wy - 1; axis = 1; }                          // d == 0
    float j = Hash01(kx, ky, seed + salt + (unsigned)axis * 977u) * 2.f - 1.f;
    float along = 128.f + j * 70.f;
    switch (d) {
        case 2: return { M, along };
        case 6: return { 0.f, along };
        case 4: return { along, M };
        default: return { along, 0.f };
    }
}

Pt CenterNode(int wx, int wy, unsigned int seed, unsigned int salt, bool pinned)
{
    if (pinned) return { 128.f, 128.f };
    float jx = Hash01(wx, wy, seed + salt) * 2.f - 1.f;
    float jy = Hash01(wx, wy, seed + salt + 313u) * 2.f - 1.f;
    return { 128.f + jx * 32.f, 128.f + jy * 32.f };
}

bool InMap(LocalMap* m, int x, int y) { return x >= 0 && y >= 0 && x < m->width && y < m->height; }

bool IsObstacle(char t)
{
    return t == TILE_TREE || t == TILE_PINE || t == TILE_CACTUS || t == TILE_BOULDER;
}

void StampRiver(LocalMap* m, float cx, float cy, float radius, bool ford, int biomeHint)
{
    int r = (int)ceilf(radius) + 2;
    for (int y = (int)cy - r; y <= (int)cy + r; y++) for (int x = (int)cx - r; x <= (int)cx + r; x++) {
        if (!InMap(m, x, y)) continue;
        float d = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
        char& t = m->tiles[y][x];
        if (d <= radius) {
            if (TileIsWater(t) && t != TILE_SHALLOW) continue;     // never un-deepen existing water
            bool core = (d <= radius * 0.55f) && radius >= 1.8f && !ford;
            if (core) t = TILE_DEEP;
            else if (t != TILE_DEEP) t = TILE_SHALLOW;
        } else if (d <= radius + 1.5f) {
            if (IsObstacle(t) || t == TILE_ROCK) t = (biomeHint == BIOME_DESERT) ? TILE_SAND : TILE_GRASS;   // clear the banks
        }
    }
}

void CarveRiverSegment(LocalMap* m, LNoise& N, Pt a, Pt b, float wa, float wb, int wx, int wy, int dirIdx)
{
    float dx = b.x - a.x, dy = b.y - a.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.f) return;
    float px = -dy / len, py = dx / len;
    float amp = std::min(30.f, len * 0.17f);
    float key = (float)(HashCell(wx, wy, N.seed + 1000u + (unsigned)dirIdx) & 1023);
    float fordPhase = (float)(HashCell(wx, wy, N.seed + 2000u + (unsigned)dirIdx) % 60);
    int steps = (int)ceilf(len * 1.2f);
    for (int s = 0; s <= steps; s++) {
        float t = (float)s / steps;
        float env = sinf(3.14159265f * t);
        float off = amp * env * N.wobble.GetNoise(t * 1.4f, key) * 2.f;
        float x = a.x + dx * t + px * off, y = a.y + dy * t + py * off;
        float width = Mix(wa, wb, t);
        bool ford = width >= 3.f && fmodf(t * len + fordPhase, 62.f) < 7.f;
        int bi = InMap(m, (int)x, (int)y) ? m->biome[(int)y * m->width + (int)x] : BIOME_GRASSLAND;
        StampRiver(m, x, y, width * 0.5f, ford, bi);
    }
}

void StampRoad(LocalMap* m, float cx, float cy, float radius)
{
    int r = (int)ceilf(radius) + 2;
    for (int y = (int)cy - r; y <= (int)cy + r; y++) for (int x = (int)cx - r; x <= (int)cx + r; x++) {
        if (!InMap(m, x, y)) continue;
        float d = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
        char& t = m->tiles[y][x];
        if (d <= radius) {
            if (TileIsWater(t)) t = TILE_BRIDGE;
            else if (t != TILE_BRIDGE && t != TILE_WALL) t = TILE_ROAD;
        } else if (d <= radius + 1.3f && IsObstacle(t)) {
            t = TILE_GRASS;                                    // verges stay clear of trees
        }
    }
}

void CarveRoadSegment(LocalMap* m, LNoise& N, Pt a, Pt b, int wx, int wy, int dirIdx, float width)
{
    float dx = b.x - a.x, dy = b.y - a.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.f) return;
    float px = -dy / len, py = dx / len;
    float amp = std::min(11.f, len * 0.09f);
    float key = (float)(HashCell(wx, wy, N.seed + 3000u + (unsigned)dirIdx) & 1023);
    int steps = (int)ceilf(len * 1.2f);
    for (int s = 0; s <= steps; s++) {
        float t = (float)s / steps;
        float env = sinf(3.14159265f * t);
        float off = amp * env * N.wobble.GetNoise(t * 1.8f + 40.f, key) * 2.f;
        StampRoad(m, a.x + dx * t + px * off, a.y + dy * t + py * off, width * 0.5f);
    }
}

// Guarantee navigability: tunnel small walled-in pockets into a larger walkable area.
void RepairConnectivity(LocalMap* m)
{
    const int W = m->width, H = m->height, N = W * H;
    std::vector<int> label(N, -1);
    std::vector<int> size;
    std::vector<int> stack;
    auto walk = [&](int i) { return TileWalkable(m->tiles[i / W][i % W]); };
    static const int DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };

    for (int i = 0; i < N; i++) {
        if (label[i] >= 0 || !walk(i)) continue;
        int id = (int)size.size(), cnt = 0;
        stack.clear(); stack.push_back(i); label[i] = id;
        while (!stack.empty()) {
            int c = stack.back(); stack.pop_back(); cnt++;
            int x = c % W, y = c / W;
            for (int d = 0; d < 4; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                int ni = ny * W + nx;
                if (label[ni] >= 0 || !walk(ni)) continue;
                label[ni] = id; stack.push_back(ni);
            }
        }
        size.push_back(cnt);
    }
    if (size.size() <= 1) return;

    const int BIG = 9000;
    std::vector<int> order(size.size());
    for (size_t i = 0; i < order.size(); i++) order[i] = (int)i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return size[a] < size[b]; });

    std::vector<int> cellsOf;                 // cells of the pocket being processed
    std::vector<int> dist(N, -1), parent(N, -1), touched;
    for (int id : order) {
        if (size[id] >= BIG || size[id] == 0) continue;
        cellsOf.clear();
        for (int i = 0; i < N; i++) if (label[i] == id) cellsOf.push_back(i);   // O(N) per pocket; pockets are few
        std::vector<int> q;
        for (int c : cellsOf) { dist[c] = 0; parent[c] = -1; touched.push_back(c); q.push_back(c); }
        int hit = -1;
        for (size_t qi = 0; qi < q.size() && hit < 0; qi++) {
            int c = q[qi]; int x = c % W, y = c / W;
            if (dist[c] >= 48) continue;
            for (int d = 0; d < 4; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                int ni = ny * W + nx;
                if (dist[ni] >= 0) continue;
                if (m->tiles[ny][nx] == TILE_DEEP) continue;                 // can't tunnel through deep water
                dist[ni] = dist[c] + 1; parent[ni] = c; touched.push_back(ni);
                if (label[ni] >= 0 && label[ni] != id) { hit = ni; break; }
                q.push_back(ni);
            }
        }
        if (hit >= 0) {
            int target = label[hit];
            for (int c = parent[hit]; c >= 0 && label[c] != id; c = parent[c]) {
                int x = c % W, y = c / W;
                char& t = m->tiles[y][x];
                if (!TileWalkable(t)) {
                    BiomeType b = (BiomeType)m->biome[c];
                    t = (b == BIOME_DESERT) ? TILE_SAND : ((b == BIOME_MOUNTAINS || b == BIOME_SNOW_PEAKS) ? TILE_HILL : ((b == BIOME_GLACIER) ? TILE_SNOW : TILE_GRASS));
                }
                label[c] = target; size[target]++;
            }
            for (int c : cellsOf) label[c] = target;
            size[target] += size[id]; size[id] = 0;
        }
        for (int c : touched) { dist[c] = -1; parent[c] = -1; }
        touched.clear();
    }
}

} // namespace

// ------------------------------------------------------------------------------------------------
static char GroundFor(LocalMap* map, int x, int y)
{
    BiomeType b = map->biome ? (BiomeType)map->biome[y * map->width + x] : BIOME_GRASSLAND;
    if (b == BIOME_DESERT) return TILE_SAND;
    if (b == BIOME_GLACIER || b == BIOME_SNOW_PEAKS) return TILE_SNOW;
    return TILE_GRASS;
}

// Used by structure placement to clear a footprint: removes trees/rocks/mud/hills but leaves water,
// roads, sand and snow alone, so a castle lawn never erases a river or flattens a beach.
void ClearTileKeepWater(LocalMap* map, int x, int y)
{
    if (!InMap(map, x, y)) return;
    char& t = map->tiles[y][x];
    if (IsObstacle(t) || t == TILE_ROCK || t == TILE_MUD || t == TILE_HILL) t = GroundFor(map, x, y);
}

void ClearObstaclesInRect(LocalMap* map, int x0, int y0, int x1, int y1)
{
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        if (!InMap(map, x, y)) continue;
        char& t = map->tiles[y][x];
        if (IsObstacle(t) || t == TILE_ROCK || t == TILE_MUD || t == TILE_HILL) t = GroundFor(map, x, y);
    }
}

void GenerateLocalTerrain(LocalMap* map, int wx, int wy)
{
    const int W = map->width, H = map->height;
    LNoise& N = Noise(worldSeed);
    const WorldTile& wt = worldMap[wy][wx];
    const unsigned int seed = worldSeed;

    if (!map->biome) map->biome = (unsigned char*)calloc((size_t)W * H, 1);

    // 3x3 neighbourhood of world-tile fields
    Field nb[3][3];
    for (int oy = -1; oy <= 1; oy++) for (int ox = -1; ox <= 1; ox++) nb[oy + 1][ox + 1] = TileField(wx + ox, wy + oy);

    const float settleR = SettlementRadius(wt.locationType);
    const bool isCaveLike = (wt.locationType == LOCATION_CAVE || wt.locationType == LOCATION_DUNGEON);
    const float mountainness = Smooth(0.55f, 0.80f, nb[1][1].e);

    for (int ly = 0; ly < H; ly++) {
        for (int lx = 0; lx < W; lx++) {
            const int px = wx * W + lx, py = wy * H + ly;     // global cell coordinates

            // continuous fields: interpolate between the four nearest world-tile centres
            float u = (lx + 0.5f) / W - 0.5f, v = (ly + 0.5f) / H - 0.5f;
            int x0 = (u < 0.f) ? 0 : 1, y0 = (v < 0.f) ? 0 : 1;
            float fx = (u < 0.f) ? u + 1.f : u, fy = (v < 0.f) ? v + 1.f : v;
            fx = fx * fx * (3.f - 2.f * fx); fy = fy * fy * (3.f - 2.f * fy);
            Field f = Bilerp(nb[y0][x0], nb[y0][x0 + 1], nb[y0 + 1][x0], nb[y0 + 1][x0 + 1], fx, fy);

            // local detail
            float nHills = N.hills.GetNoise(px * 0.010f, py * 0.010f);
            float nFine  = N.detail.GetNoise(px * 0.045f, py * 0.045f);
            Ctx c;
            c.nA = N.patch.GetNoise(px * 0.020f, py * 0.020f);
            c.nC = N.detail.GetNoise(px * 0.075f + 100.f, py * 0.075f - 50.f);
            // keep shorelines calm: strong small-scale noise on a gentle shelf turns coasts into confetti
            float landward = Smooth(SEA_LEVEL - 0.03f, SEA_LEVEL + 0.14f, f.e);
            float rough = (0.035f + 0.075f * mountainness) * landward + 0.012f;
            c.elevL = f.e + nHills * rough + nFine * (0.010f * landward + 0.003f);
            c.tempL = Clamp01(f.t - (c.elevL - f.e) * 0.5f + c.nA * 0.025f);
            c.rainL = Clamp01(f.r + N.moist.GetNoise(px * 0.012f, py * 0.012f) * 0.22f);
            c.r1 = Hash01(px, py, seed + 7u);
            c.r2 = Hash01(px, py, seed + 91u);
            c.clearing = 0.f;
            c.ridge = 0.f;
            c.trail = N.trail.GetNoise(px * 0.011f, py * 0.011f);

            // settlements & dungeon mouths get a flattened, cleared area
            if (settleR > 0.f) {
                float dist = sqrtf((lx - 128.f) * (lx - 128.f) + (ly - 128.f) * (ly - 128.f));
                dist *= 1.f + 0.28f * c.nA;                      // irregular edge, not a perfect circle
                float fl = 1.f - Smooth(settleR * 0.55f, settleR, dist);
                if (fl > 0.f) {
                    float target = isCaveLike ? Clamp01(std::max(0.60f, std::min(0.66f, nb[1][1].e)))
                                              : std::min(0.55f, std::max(nb[1][1].e, SEA_LEVEL + 0.06f));
                    c.elevL = Mix(c.elevL, target, fl);
                    // use the tile's own smooth climate (no noise) so a settlement is one coherent biome, not blotchy
                    c.rainL = Mix(c.rainL, std::max(0.30f, std::min(0.55f, f.r)), fl);
                    c.tempL = Mix(c.tempL, std::max(f.t, 0.15f), fl);
                    c.clearing = fl;
                }
            }

            BiomeType b = ClassifyBiome(c.elevL, c.tempL, c.rainL);
            map->biome[ly * W + lx] = (unsigned char)b;

            char tile;
            if (b == BIOME_DEEP_OCEAN || b == BIOME_OCEAN || c.elevL < SEA_LEVEL) {
                tile = (c.elevL < SEA_LEVEL - 0.045f) ? TILE_DEEP : TILE_SHALLOW;
                map->biome[ly * W + lx] = (unsigned char)((tile == TILE_DEEP) ? BIOME_DEEP_OCEAN : BIOME_OCEAN);
            } else {
                if (b == BIOME_MOUNTAINS || b == BIOME_SNOW_PEAKS)
                    c.ridge = N.ridge.GetNoise(px * 0.022f, py * 0.022f);
                tile = PickLand(b, c);
            }
            map->tiles[ly][lx] = tile;
        }
        map->tiles[ly][W] = '\0';
    }

    // ---- rivers (drawn first so roads can bridge them) ----
    if (wt.riverMask) {
        Pt centre = CenterNode(wx, wy, seed, 5000u, false);
        float own = (float)std::max<int>(2, wt.riverWidth);
        for (int d = 0; d < 8; d++) {
            if (!(wt.riverMask & (1 << d))) continue;
            int nx = wx + DIR_DX[d], ny = wy + DIR_DY[d];
            float edgeW = own;
            if (nx >= 0 && ny >= 0 && nx < currentMapWidth && ny < currentMapHeight && worldMap[ny][nx].riverWidth > 0)
                edgeW = (float)std::min<int>(own, worldMap[ny][nx].riverWidth);     // both tiles agree: the narrower (upstream) width
            Pt edge = EdgePoint(wx, wy, d, seed, 6000u);
            CarveRiverSegment(map, N, centre, edge, own, edgeW, wx, wy, d);
        }
    }

    // ---- roads ----
    if (wt.roadMask) {
        bool pinned = (wt.locationType != LOCATION_NONE);
        Pt centre = CenterNode(wx, wy, seed, 7000u, pinned);
        for (int d = 0; d < 8; d++) {
            if (!(wt.roadMask & (1 << d))) continue;
            Pt edge = EdgePoint(wx, wy, d, seed, 8000u);
            CarveRoadSegment(map, N, centre, edge, wx, wy, d, 2.4f);
        }
    }

    RepairConnectivity(map);
}
