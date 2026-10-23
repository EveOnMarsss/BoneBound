#include "WorldGen.h"
#include "FastNoiseLite.h"
#include "LocationNames.h"
#include "WorldRng.h"

#include <vector>
#include <queue>
#include <algorithm>
#include <atomic>
#include <thread>
#include <string>
#include <set>
#include <cmath>
#include <cstring>
#include <functional>

// ---------------------------------------------------------------------------
// progress / threading state
// ---------------------------------------------------------------------------
static std::atomic<int>  g_progress(0);
static std::atomic<int>  g_stage(0);
static std::atomic<bool> g_done(false);
static std::thread*      g_thread = NULL;
static int               g_startX = 0, g_startY = 0;

static const char* STAGE_NAMES[] = {
    "Raising continents...",
    "Shaping mountains and valleys...",
    "Blowing the winds...",
    "Letting the rain fall...",
    "Carving rivers...",
    "Growing forests and deserts...",
    "Founding villages and strongholds...",
    "Laying roads...",
    "Finishing touches..."
};

static void Stage(int stage, int progress) { g_stage.store(stage); g_progress.store(progress); }

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------
static inline float Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
static inline float Smooth(float a, float b, float x) {
    float t = Clamp01((x - a) / (b - a));
    return t * t * (3.f - 2.f * t);
}

BiomeType ClassifyBiome(float e, float t, float r)
{
    if (e < SEA_LEVEL - 0.13f) return BIOME_DEEP_OCEAN;
    if (e < SEA_LEVEL)         return BIOME_OCEAN;

    // altitude bands
    float snowLine = 0.62f + 0.30f * t;          // warmer climates need to climb higher for snow
    if (e >= 0.84f) return (e > snowLine - 0.06f) ? BIOME_SNOW_PEAKS : BIOME_MOUNTAINS;
    if (e >= 0.70f) return (e > snowLine + 0.02f) ? BIOME_SNOW_PEAKS : BIOME_MOUNTAINS;
    if (t < 0.10f)  return BIOME_GLACIER;
    if (e >= 0.58f) return BIOME_HILLS;

    // lowland climate (a Whittaker-style temperature / rainfall chart)
    if (t > 0.72f) {
        if (r < 0.18f) return BIOME_DESERT;
        if (r < 0.42f) return BIOME_SAVANNA;
        if (r < 0.64f) return BIOME_FOREST;
        if (r > 0.88f && e < SEA_LEVEL + 0.05f) return BIOME_SWAMP;
        return BIOME_JUNGLE;
    }
    if (t > 0.48f) {
        if (r < 0.15f) return (t > 0.60f) ? BIOME_DESERT : BIOME_GRASSLAND;
        if (r < 0.47f) return BIOME_GRASSLAND;
        if (r > 0.86f && e < SEA_LEVEL + 0.05f) return BIOME_SWAMP;
        return BIOME_FOREST;
    }
    if (t > 0.28f) return (r >= 0.35f) ? BIOME_TAIGA : BIOME_GRASSLAND;
    return (t > 0.18f && r >= 0.40f) ? BIOME_TAIGA : BIOME_TUNDRA;
}

namespace {

struct Noises {
    FastNoiseLite cont, warpA, warpB, ridge, range, hills, tempN, rainN;
    explicit Noises(unsigned int seed)
    {
        int s = (int)seed;
        cont.SetSeed(s);        cont.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        cont.SetFractalType(FastNoiseLite::FractalType_FBm);   cont.SetFractalOctaves(5);
        cont.SetFrequency(1.0f);

        warpA.SetSeed(s + 101); warpA.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); warpA.SetFrequency(1.0f);
        warpB.SetSeed(s + 202); warpB.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); warpB.SetFrequency(1.0f);

        ridge.SetSeed(s + 303); ridge.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        ridge.SetFractalType(FastNoiseLite::FractalType_Ridged); ridge.SetFractalOctaves(4);
        ridge.SetFrequency(1.0f);

        range.SetSeed(s + 404); range.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        range.SetFractalType(FastNoiseLite::FractalType_FBm); range.SetFractalOctaves(2); range.SetFrequency(1.0f);

        hills.SetSeed(s + 505); hills.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        hills.SetFractalType(FastNoiseLite::FractalType_FBm); hills.SetFractalOctaves(4); hills.SetFrequency(1.0f);

        tempN.SetSeed(s + 606); tempN.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2); tempN.SetFrequency(1.0f);

        rainN.SetSeed(s + 707); rainN.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        rainN.SetFractalType(FastNoiseLite::FractalType_FBm); rainN.SetFractalOctaves(3); rainN.SetFrequency(1.0f);
    }
};

struct Site { int x, y; LocationType type; };

struct Generator {
    int W, H, N;
    unsigned int seed;
    WorldTile** map;

    std::vector<float> elev, temp, rain, filled, acc;
    std::vector<int8_t> dir;
    std::vector<uint8_t> riverMask, roadMask, riverWidth, lake, coastal, nearRiver, nearOcean, nearMtn;
    std::vector<BiomeType> biome;
    std::vector<Site> sites;
    std::vector<int> siteAt;           // index into sites or -1
    float riverThreshold;
    Rng rng;

    Generator(int w, int h, unsigned int s, WorldTile** m)
        : W(w), H(h), N(w * h), seed(s), map(m), rng(s ^ 0xA5A5F00Du) {}

    inline int I(int x, int y) const { return y * W + x; }
    inline bool In(int x, int y) const { return x >= 0 && y >= 0 && x < W && y < H; }
    inline bool IsOcean(int i) const { return elev[i] < SEA_LEVEL && !lake[i]; }
    inline bool IsWater(int i) const { return elev[i] < SEA_LEVEL; }

    float LatitudeAt(float ny) const
    {
        float span = Clamp01(log2f((float)std::max(W, H) / 16.f) / 4.f);
        float latScale = 0.35f + 0.65f * span;     // small worlds only cover a temperate slice of the planet
        return fabsf(ny * 2.f - 1.f) * latScale + (1.f - latScale) * 0.5f;   // 0 equator .. 1 pole
    }

    // ------------------------------------------------------------------ elevation
    void GenElevation()
    {
        Noises n(seed);
        std::vector<float> cont(N);
        float F = 1.2f + 0.5f * log2f((float)std::max(16, std::max(W, H)) / 16.f);

        Stage(0, 2);
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            float nx = (x + 0.5f) / W, ny = (y + 0.5f) / H;
            float px = nx * F + 0.30f * n.warpA.GetNoise(nx * 2.3f, ny * 2.3f);     // gentle domain warp (in noise space)
            float py = ny * F + 0.30f * n.warpB.GetNoise(nx * 2.3f + 9.1f, ny * 2.3f + 4.7f);
            float c = n.cont.GetNoise(px, py) * 0.5f + 0.5f;
            float dx = fabsf(nx * 2.f - 1.f), dy = fabsf(ny * 2.f - 1.f) * 0.78f;   // land may reach the polar edges
            float d = powf(powf(dx, 4.f) + powf(dy, 4.f), 0.25f);
            float edge = 1.f - Smooth(0.70f, 1.0f, d);
            cont[I(x, y)] = c + (edge - 1.f);
        }

        // choose the sea level so a fixed fraction of the world is land
        float landFraction = (N <= 64) ? 0.52f : (N <= 1024 ? 0.46f : 0.40f);
        std::vector<float> sorted(cont);
        size_t k = (size_t)((1.f - landFraction) * (N - 1));
        std::nth_element(sorted.begin(), sorted.begin() + k, sorted.end());
        float sea = sorted[k];
        float minV = *std::min_element(cont.begin(), cont.end());
        float maxV = *std::max_element(cont.begin(), cont.end());

        Stage(1, 12);
        elev.assign(N, 0.f);
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            int i = I(x, y);
            float nx = (x + 0.5f) / W, ny = (y + 0.5f) / H;
            float v = cont[i];
            if (v < sea) {
                float d = (sea - v) / std::max(0.0001f, sea - minV);         // 0 coast .. 1 abyss
                elev[i] = SEA_LEVEL * (1.f - powf(d, 1.25f) * 0.97f);        // wide shallow shelf
            } else {
                float land = (v - sea) / std::max(0.0001f, maxV - sea);      // 0 coast .. 1 deep inland
                float base = powf(land, 0.75f) * 0.34f;
                float hills = n.hills.GetNoise(nx * F * 3.2f, ny * F * 3.2f) * 0.07f * Smooth(0.02f, 0.25f, land);
                float px = nx * F + 0.30f * n.warpA.GetNoise(nx * 2.3f, ny * 2.3f);
                float py = ny * F + 0.30f * n.warpB.GetNoise(nx * 2.3f + 9.1f, ny * 2.3f + 4.7f);
                float r = Clamp01(n.ridge.GetNoise(px * 1.8f, py * 1.8f) * 0.5f + 0.5f);
                float rangeMask = Smooth(0.30f, 0.62f, n.range.GetNoise(nx * F * 0.9f + 31.f, ny * F * 0.9f + 17.f) * 0.5f + 0.5f);
                float mountain = powf(r, 1.7f) * rangeMask * 0.95f * Smooth(0.06f, 0.35f, land);
                float above = base + hills + mountain;
                elev[i] = std::min(1.f, SEA_LEVEL + above * (1.f - SEA_LEVEL));
            }
        }
    }

    // ------------------------------------------------------------------ climate
    void GenClimate()
    {
        Noises n(seed);
        float F = 1.2f + 0.5f * log2f((float)std::max(16, std::max(W, H)) / 16.f);
        Stage(2, 22);

        temp.assign(N, 0.f);
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            int i = I(x, y);
            float nx = (x + 0.5f) / W, ny = (y + 0.5f) / H;
            float lat = LatitudeAt(ny) + n.tempN.GetNoise(nx * 4.f, ny * 4.f) * 0.04f;
            float t = cosf(Clamp01(lat) * 1.5708f);
            t -= std::max(0.f, elev[i] - SEA_LEVEL) * 0.50f;            // thinner air is colder
            t += n.tempN.GetNoise(nx * 12.f + 50.f, ny * 12.f) * 0.03f;
            temp[i] = Clamp01(t);
        }

        // --- prevailing winds carry moisture from the oceans; mountains wring it out ---
        Stage(3, 30);
        std::vector<float> precip(N, 0.f);
        for (int y = 0; y < H; y++) {
            float lat = LatitudeAt((y + 0.5f) / H);
            int wind = (lat < 0.30f) ? -1 : (lat < 0.65f ? +1 : -1);   // trades / westerlies / polar easterlies
            int x0 = wind > 0 ? 0 : W - 1;
            float m = 0.6f;
            float prevE = elev[I(x0, y)];
            for (int k = 0; k < W; k++) {
                int x = x0 + wind * k;
                int i = I(x, y);
                float e = elev[i];
                if (e < SEA_LEVEL) {
                    m = std::min(1.3f, m + 0.06f + 0.18f * cosf(Clamp01(lat) * 1.5708f));   // evaporation
                    precip[i] = m * 0.25f;
                } else {
                    float rise = std::max(0.f, e - std::max(prevE, SEA_LEVEL));
                    float p = m * (0.05f + rise * 9.0f + (e - SEA_LEVEL) * 0.12f);
                    p = std::min(p, m * 0.55f);
                    precip[i] = p;
                    m = std::max(0.f, m - p * 0.70f) + 0.02f;         // a little recycling from vegetation
                }
                prevE = e;
            }
        }

        // latitude belts: wet tropics, dry subtropics (deserts), stormy mid-latitudes, dry poles
        for (int y = 0; y < H; y++) {
            float lat = LatitudeAt((y + 0.5f) / H);
            float lf = 1.f;
            lf += 0.35f * (1.f - Smooth(0.05f, 0.20f, lat));
            lf -= 0.50f * (Smooth(0.15f, 0.28f, lat) - Smooth(0.38f, 0.55f, lat));
            lf += 0.10f * (Smooth(0.45f, 0.60f, lat) - Smooth(0.70f, 0.80f, lat));
            lf -= 0.40f * Smooth(0.75f, 1.0f, lat);
            for (int x = 0; x < W; x++) precip[I(x, y)] *= lf;
        }

        // normalise: blend of absolute amount and rank among land cells, so each world has a believable
        // spread of wet and dry lands regardless of how the winds happened to fall
        std::vector<std::pair<float,int>> landRank;
        for (int i = 0; i < N; i++) if (elev[i] >= SEA_LEVEL) landRank.push_back({ precip[i], i });
        std::sort(landRank.begin(), landRank.end());
        float p80 = landRank.empty() ? 0.3f : std::max(0.05f, landRank[(size_t)(landRank.size() * 0.80f)].first);
        std::vector<float> rankV(N, 0.f);
        for (size_t k = 0; k < landRank.size(); k++) rankV[landRank[k].second] = (float)k / std::max<size_t>(1, landRank.size() - 1);
        for (int i = 0; i < N; i++) precip[i] = (elev[i] >= SEA_LEVEL) ? Clamp01(0.55f * Clamp01(precip[i] / p80) + 0.45f * rankV[i]) : Clamp01(precip[i] / p80);

        // soften horizontal streaking from the row sweeps
        for (int pass = 0; pass < 2; pass++) {
            std::vector<float> tmp(precip);
            for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
                float sum = 0; int cnt = 0;
                for (int oy = -1; oy <= 1; oy++) for (int ox = -1; ox <= 1; ox++) {
                    int nx = x + ox, ny = y + oy;
                    if (!In(nx, ny)) continue;
                    sum += tmp[I(nx, ny)]; cnt++;
                }
                precip[I(x, y)] = sum / cnt;
            }
        }

        rain.assign(N, 0.f);
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            int i = I(x, y);
            float nx = (x + 0.5f) / W, ny = (y + 0.5f) / H;
            rain[i] = Clamp01(precip[i] + n.rainN.GetNoise(nx * F * 2.5f, ny * F * 2.5f) * 0.14f);
        }
    }

    // ------------------------------------------------------------------ hydrology
    void GenHydrology()
    {
        Stage(4, 42);
        filled = elev;
        dir.assign(N, -1);
        lake.assign(N, 0);
        std::vector<uint8_t> closed(N, 0);

        struct PQ { float e; int i; };
        struct Cmp { bool operator()(const PQ& a, const PQ& b) const { return a.e > b.e; } };
        std::priority_queue<PQ, std::vector<PQ>, Cmp> pq;

        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            int i = I(x, y);
            bool border = (x == 0 || y == 0 || x == W - 1 || y == H - 1);
            if (elev[i] < SEA_LEVEL || border) { closed[i] = 1; pq.push({ elev[i], i }); }
        }
        // priority-flood: every land cell gets a downhill path to the sea, pits become lakes
        while (!pq.empty()) {
            PQ c = pq.top(); pq.pop();
            int cx = c.i % W, cy = c.i / W;
            for (int d = 0; d < 8; d++) {
                int nx = cx + DIR_DX[d], ny = cy + DIR_DY[d];
                if (!In(nx, ny)) continue;
                int ni = I(nx, ny);
                if (closed[ni]) continue;
                closed[ni] = 1;
                float ne = elev[ni];
                if (ne <= filled[c.i]) ne = filled[c.i] + 1e-4f;
                filled[ni] = ne;
                dir[ni] = (int8_t)((d + 4) & 7);
                pq.push({ ne, ni });
            }
        }

        // Slopes per tile get shallower as the world gets bigger, so the pit depth that counts as a lake
        // must grow with the world size or large worlds drown in puddles.
        const float lakeDepth = 0.012f * std::max(1.f, (float)std::max(W, H) / 256.f) * std::max(1.f, (float)std::max(W, H) / 256.f);
        for (int i = 0; i < N; i++)
            if (elev[i] >= SEA_LEVEL + 0.01f && filled[i] - elev[i] > lakeDepth) lake[i] = 1;

        // flow accumulation, highest ground first
        std::vector<int> order(N);
        for (int i = 0; i < N; i++) order[i] = i;
        std::sort(order.begin(), order.end(), [&](int a, int b) { return filled[a] > filled[b]; });
        acc.assign(N, 0.f);
        for (int i = 0; i < N; i++) acc[i] = (elev[i] < SEA_LEVEL) ? 0.f : (0.2f + rain[i]);
        for (int idx = 0; idx < N; idx++) {
            int i = order[idx];
            if (dir[i] < 0) continue;
            int j = I(i % W + DIR_DX[dir[i]], i / W + DIR_DY[dir[i]]);
            acc[j] += acc[i];
        }

        riverThreshold = 2.5f + 0.0006f * (float)std::min(N, 65536);
        if (N > 65536) riverThreshold *= sqrtf((float)N / 65536.f);   // bigger worlds: keep river density per area similar
        riverMask.assign(N, 0);
        riverWidth.assign(N, 0);
        for (int i = 0; i < N; i++) {
            if (elev[i] < SEA_LEVEL || acc[i] < riverThreshold || dir[i] < 0) continue;
            int d = dir[i];
            int j = I(i % W + DIR_DX[d], i / W + DIR_DY[d]);
            riverMask[i] |= (uint8_t)(1 << d);
            riverMask[j] |= (uint8_t)(1 << ((d + 4) & 7));
        }
        for (int i = 0; i < N; i++) {
            if (!riverMask[i]) continue;
            float w = 2.0f + 2.2f * log2f(acc[i] / riverThreshold + 0.5f);
            riverWidth[i] = (uint8_t)std::max(2.f, std::min(14.f, w));
        }

        // lakes sit below sea level in the elevation field so local maps render them as water
        for (int i = 0; i < N; i++) if (lake[i]) elev[i] = SEA_LEVEL - 0.08f;
    }

    // ------------------------------------------------------------------ biomes
    void GenBiomes()
    {
        Stage(5, 55);
        biome.assign(N, BIOME_GRASSLAND);
        coastal.assign(N, 0); nearRiver.assign(N, 0); nearOcean.assign(N, 0); nearMtn.assign(N, 0);

        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            int i = I(x, y);
            for (int oy = -1; oy <= 1; oy++) for (int ox = -1; ox <= 1; ox++) {
                int nx = x + ox, ny = y + oy;
                if (!In(nx, ny)) continue;
                int j = I(nx, ny);
                if (elev[j] < SEA_LEVEL) coastal[i] = 1;
                if (IsOcean(j)) nearOcean[i] = 1;
                if (riverMask[j] && !lake[j]) nearRiver[i] = 1;
            }
        }
        for (int i = 0; i < N; i++) {
            if (lake[i]) { biome[i] = BIOME_LAKE; continue; }
            BiomeType b = ClassifyBiome(elev[i], temp[i], rain[i]);
            if (elev[i] >= SEA_LEVEL && coastal[i] && elev[i] < SEA_LEVEL + 0.045f && temp[i] > 0.12f && b != BIOME_SWAMP)
                b = BIOME_BEACH;
            biome[i] = b;
        }
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            int i = I(x, y);
            for (int oy = -1; oy <= 1; oy++) for (int ox = -1; ox <= 1; ox++) {
                int nx = x + ox, ny = y + oy;
                if (!In(nx, ny)) continue;
                BiomeType b = biome[I(nx, ny)];
                if (b == BIOME_MOUNTAINS || b == BIOME_SNOW_PEAKS) nearMtn[i] = 1;
            }
        }
    }

    // ------------------------------------------------------------------ settlements
    bool IsLandSite(int i) const
    {
        BiomeType b = biome[i];
        return b != BIOME_DEEP_OCEAN && b != BIOME_OCEAN && b != BIOME_LAKE && b != BIOME_GLACIER &&
               b != BIOME_SNOW_PEAKS && siteAt[i] < 0;
    }

    // sameDist: spacing to sites of the same type; anyDist: small gap to every other kind of site
    bool FarFromSites(int x, int y, LocationType type, float sameDist, float anyDist) const
    {
        int R = (int)ceilf(std::max(sameDist, anyDist));
        for (int oy = -R; oy <= R; oy++) for (int ox = -R; ox <= R; ox++) {
            int nx = x + ox, ny = y + oy;
            if (!In(nx, ny)) continue;
            int si = siteAt[I(nx, ny)];
            if (si < 0) continue;
            float d = sqrtf((float)(ox * ox + oy * oy));
            if (d < anyDist) return false;
            if (sites[si].type == type && d < sameDist) return false;
        }
        return true;
    }

    int CountNear(int x, int y, int R, LocationType type) const
    {
        int c = 0;
        for (int oy = -R; oy <= R; oy++) for (int ox = -R; ox <= R; ox++) {
            int nx = x + ox, ny = y + oy;
            if (!In(nx, ny)) continue;
            int s = siteAt[I(nx, ny)];
            if (s >= 0 && sites[s].type == type) c++;
        }
        return c;
    }

    static float VillageBiomeScore(BiomeType b)
    {
        switch (b) {
            case BIOME_GRASSLAND: return 1.0f;  case BIOME_FOREST: return 0.75f; case BIOME_SAVANNA: return 0.6f;
            case BIOME_BEACH: return 0.55f;     case BIOME_HILLS: return 0.5f;   case BIOME_TAIGA: return 0.4f;
            case BIOME_JUNGLE: return 0.35f;    case BIOME_DESERT: return 0.2f;  case BIOME_SWAMP: return 0.2f;
            case BIOME_TUNDRA: return 0.15f;    case BIOME_MOUNTAINS: return 0.0f; default: return 0.f;
        }
    }

    template <typename ScoreFn>
    int PlaceSites(LocationType type, int count, float minDist, ScoreFn score)
    {
        struct Cand { float s; int i; };
        std::vector<Cand> cands;
        for (int y = 1; y < H - 1; y++) for (int x = 1; x < W - 1; x++) {
            int i = I(x, y);
            if (!IsLandSite(i)) continue;
            float s = score(x, y, i);
            if (s > 0.f) cands.push_back({ s + rng.f() * 0.35f, i });
        }
        std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.s > b.s; });
        int placed = 0;
        for (size_t c = 0; c < cands.size() && placed < count; c++) {
            int x = cands[c].i % W, y = cands[c].i / W;
            if (!FarFromSites(x, y, type, minDist, 1.5f)) continue;
            siteAt[cands[c].i] = (int)sites.size();
            sites.push_back({ x, y, type });
            placed++;
        }
        return placed;
    }

    void GenSites(int* startX, int* startY)
    {
        Stage(6, 62);
        siteAt.assign(N, -1);
        int L = 0;
        for (int i = 0; i < N; i++) if (IsLandSite(i) || siteAt[i] >= 0) L++;
        L = 0; for (int i = 0; i < N; i++) if (biome[i] != BIOME_DEEP_OCEAN && biome[i] != BIOME_OCEAN && biome[i] != BIOME_LAKE) L++;

        auto cnt = [&](float rate, int minimum, int minLand) { return L >= minLand ? std::max(minimum, (int)lroundf(L * rate)) : 0; };
        int nVillage = std::max(2, (int)lroundf(L * 0.035f));
        if (L < 12) nVillage = 1;

        float vDist = std::max(2.f, 0.6f * sqrtf((float)L / std::max(1, nVillage)));
        PlaceSites(LOCATION_VILLAGE, nVillage, vDist, [&](int x, int y, int i) {
            float s = VillageBiomeScore(biome[i]);
            if (s <= 0.f) return 0.f;
            if (nearRiver[i]) s += 0.8f;
            if (nearOcean[i]) s += 0.5f;
            s -= fabsf(elev[i] - (SEA_LEVEL + 0.10f)) * 1.5f;      // prefer gentle lowlands
            (void)x; (void)y;
            return s;
        });

        PlaceSites(LOCATION_CASTLE, cnt(0.005f, 1, 80), vDist * 2.5f, [&](int x, int y, int i) {
            float s = 0.f;
            switch (biome[i]) { case BIOME_HILLS: s = 1.0f; break; case BIOME_GRASSLAND: s = 0.7f; break; case BIOME_FOREST: s = 0.5f; break; case BIOME_MOUNTAINS: s = 0.3f; break; default: return 0.f; }
            if (nearRiver[i]) s += 0.4f;
            if (CountNear(x, y, 8, LOCATION_VILLAGE) > 0) s += 0.8f;     // lords like having subjects nearby
            return s;
        });
        PlaceSites(LOCATION_FORT, cnt(0.006f, 1, 80), vDist * 1.8f, [&](int, int, int i) {
            float s = 0.f;
            switch (biome[i]) { case BIOME_HILLS: s = 0.9f; break; case BIOME_MOUNTAINS: s = 0.6f; break; case BIOME_GRASSLAND: s = 0.4f; break; case BIOME_FOREST: s = 0.3f; break; default: return 0.f; }
            if (nearMtn[i]) s += 0.5f;
            return s;
        });
        PlaceSites(LOCATION_TEMPLE, cnt(0.004f, 1, 80), vDist * 2.0f, [&](int x, int y, int i) {
            float s = 0.f;
            switch (biome[i]) { case BIOME_HILLS: s = 0.9f; break; case BIOME_FOREST: s = 0.6f; break; case BIOME_GRASSLAND: s = 0.6f; break; case BIOME_MOUNTAINS: s = 0.5f; break; default: return 0.f; }
            if (CountNear(x, y, 2, LOCATION_VILLAGE) == 0) s += 0.3f;
            return s;
        });
        PlaceSites(LOCATION_WIZARD_TOWER, cnt(0.004f, 1, 80), vDist * 2.0f, [&](int x, int y, int i) {
            float s = 0.f;
            switch (biome[i]) { case BIOME_FOREST: s = 0.8f; break; case BIOME_HILLS: s = 0.8f; break; case BIOME_TAIGA: s = 0.6f; break; case BIOME_SWAMP: s = 0.6f; break; default: return 0.f; }
            if (CountNear(x, y, 3, LOCATION_VILLAGE) > 0) s -= 0.5f;     // wizards prefer solitude
            return s;
        });
        PlaceSites(LOCATION_RUINS, cnt(0.010f, 1, 12), vDist * 1.5f, [&](int, int, int i) {
            switch (biome[i]) { case BIOME_DESERT: case BIOME_FOREST: case BIOME_TUNDRA: case BIOME_SWAMP: return 1.0f;
                                case BIOME_GRASSLAND: case BIOME_SAVANNA: case BIOME_JUNGLE: case BIOME_TAIGA: case BIOME_HILLS: return 0.7f;
                                default: return 0.f; }
        });
        PlaceSites(LOCATION_CAVE, cnt(0.014f, 1, 12), vDist * 1.2f, [&](int, int, int i) {
            switch (biome[i]) { case BIOME_MOUNTAINS: return 1.0f; case BIOME_HILLS: return nearMtn[i] ? 0.9f : 0.5f; default: return 0.f; }
        });
        PlaceSites(LOCATION_DUNGEON, cnt(0.005f, 1, 80), vDist * 1.8f, [&](int x, int y, int i) {
            float s = 0.f;
            switch (biome[i]) { case BIOME_MOUNTAINS: s = 0.8f; break; case BIOME_SWAMP: s = 0.6f; break; case BIOME_TAIGA: s = 0.5f; break; case BIOME_FOREST: s = 0.4f; break; case BIOME_HILLS: s = 0.4f; break; default: return 0.f; }
            if (CountNear(x, y, 3, LOCATION_VILLAGE) > 0) s -= 0.4f;
            return s;
        });

        // --- choose the starting village: friendly neighbourhood (other villages close by), near water ---
        float best = -1e9f; int bi = -1;
        for (size_t s = 0; s < sites.size(); s++) {
            if (sites[s].type != LOCATION_VILLAGE) continue;
            int x = sites[s].x, y = sites[s].y, i = I(x, y);
            float sc = VillageBiomeScore(biome[i]) + (nearRiver[i] ? 0.5f : 0.f) + 0.15f * CountNear(x, y, 6, LOCATION_VILLAGE)
                     + (nearMtn[i] ? -0.3f : 0.f);
            int edge = std::min(std::min(x, y), std::min(W - 1 - x, H - 1 - y));
            if (edge < 2) sc -= 1.0f;
            if (sc > best) { best = sc; bi = (int)s; }
        }
        if (bi >= 0) { *startX = sites[bi].x; *startY = sites[bi].y; }
        else {  // no villages at all (tiny odd world) - fall back to any land
            *startX = W / 2; *startY = H / 2;
            for (int i = 0; i < N; i++) if (IsLandSite(i)) { *startX = i % W; *startY = i / W; break; }
        }
    }

    // ------------------------------------------------------------------ roads
    float RoadCost(int ci, int ni, bool diag) const
    {
        BiomeType b = biome[ni];
        float mult;
        switch (b) {
            case BIOME_DEEP_OCEAN: case BIOME_OCEAN: case BIOME_LAKE: return -1.f;     // impassable
            case BIOME_BEACH: mult = 1.1f; break;     case BIOME_DESERT: mult = 1.25f; break;
            case BIOME_SAVANNA: mult = 1.0f; break;   case BIOME_GRASSLAND: mult = 1.0f; break;
            case BIOME_FOREST: mult = 1.35f; break;   case BIOME_JUNGLE: mult = 2.0f; break;
            case BIOME_SWAMP: mult = 2.6f; break;     case BIOME_TAIGA: mult = 1.4f; break;
            case BIOME_TUNDRA: mult = 1.3f; break;    case BIOME_GLACIER: mult = 6.f; break;
            case BIOME_HILLS: mult = 1.7f; break;     case BIOME_MOUNTAINS: mult = 5.5f; break;
            case BIOME_SNOW_PEAKS: mult = 11.f; break; default: mult = 1.5f; break;
        }
        float cost = (diag ? 1.4142f : 1.f) * mult + fabsf(elev[ni] - elev[ci]) * 45.f;
        if (riverMask[ni]) cost += 3.5f;                     // needs a bridge or ford
        if (roadMask[ni]) cost *= 0.4f;                      // reuse existing roads -> natural networks
        return cost;
    }

    bool BuildRoad(int sx, int sy, int tx, int ty)
    {
        const int PAD = 8;
        int minx = std::max(0, std::min(sx, tx) - PAD), maxx = std::min(W - 1, std::max(sx, tx) + PAD);
        int miny = std::max(0, std::min(sy, ty) - PAD), maxy = std::min(H - 1, std::max(sy, ty) + PAD);
        int bw = maxx - minx + 1, bh = maxy - miny + 1;
        std::vector<float> g((size_t)bw * bh, 1e30f);
        std::vector<int> parent((size_t)bw * bh, -1);
        struct Node { float f; int x, y; };
        struct Cmp { bool operator()(const Node& a, const Node& b) const { return a.f > b.f; } };
        std::priority_queue<Node, std::vector<Node>, Cmp> open;
        auto L = [&](int x, int y) { return (y - miny) * bw + (x - minx); };
        auto hfn = [&](int x, int y) {
            float dx = (float)abs(x - tx), dy = (float)abs(y - ty);
            return 0.4f * (std::max(dx, dy) + 0.4142f * std::min(dx, dy));
        };
        g[L(sx, sy)] = 0.f;
        open.push({ hfn(sx, sy), sx, sy });
        bool found = false;
        while (!open.empty()) {
            Node cur = open.top(); open.pop();
            if (cur.x == tx && cur.y == ty) { found = true; break; }
            if (cur.f - hfn(cur.x, cur.y) > g[L(cur.x, cur.y)] + 1e-3f) continue;
            for (int d = 0; d < 8; d++) {
                int nx = cur.x + DIR_DX[d], ny = cur.y + DIR_DY[d];
                if (nx < minx || nx > maxx || ny < miny || ny > maxy) continue;
                bool diag = (d & 1) != 0;
                if (diag) {  // don't squeeze diagonally between two water tiles
                    if (IsWater(I(cur.x + DIR_DX[d], cur.y)) && IsWater(I(cur.x, cur.y + DIR_DY[d]))) continue;
                }
                float c = RoadCost(I(cur.x, cur.y), I(nx, ny), diag);
                if (c < 0.f) continue;
                float ng = g[L(cur.x, cur.y)] + c;
                if (ng < g[L(nx, ny)]) {
                    g[L(nx, ny)] = ng;
                    parent[L(nx, ny)] = L(cur.x, cur.y);
                    open.push({ ng + hfn(nx, ny), nx, ny });
                }
            }
        }
        if (!found) return false;
        int cx = tx, cy = ty;
        while (!(cx == sx && cy == sy)) {
            int p = parent[L(cx, cy)];
            int px = minx + p % bw, py = miny + p / bw;
            int ddx = cx - px, ddy = cy - py;
            for (int d = 0; d < 8; d++) {
                if (DIR_DX[d] == ddx && DIR_DY[d] == ddy) {
                    roadMask[I(px, py)] |= (uint8_t)(1 << d);
                    roadMask[I(cx, cy)] |= (uint8_t)(1 << ((d + 4) & 7));
                    break;
                }
            }
            cx = px; cy = py;
        }
        return true;
    }

    void GenRoads()
    {
        Stage(7, 75);
        roadMask.assign(N, 0);

        std::vector<int> nodes;     // sites that get proper roads
        std::vector<int> wild;      // remote sites: optional footpath
        for (size_t s = 0; s < sites.size(); s++) {
            LocationType t = sites[s].type;
            if (t == LOCATION_VILLAGE || t == LOCATION_CASTLE || t == LOCATION_FORT || t == LOCATION_TEMPLE) nodes.push_back((int)s);
            else wild.push_back((int)s);
        }

        struct Edge { float d; int a, b; };
        std::vector<Edge> edges;
        const float MAXLINK = 16.f;
        for (size_t a = 0; a < nodes.size(); a++) {
            std::vector<std::pair<float, int>> near;
            for (size_t b = 0; b < nodes.size(); b++) {
                if (a == b) continue;
                float dx = (float)(sites[nodes[a]].x - sites[nodes[b]].x), dy = (float)(sites[nodes[a]].y - sites[nodes[b]].y);
                float d = sqrtf(dx * dx + dy * dy);
                if (d <= MAXLINK) near.push_back({ d, (int)b });
            }
            std::sort(near.begin(), near.end());
            for (size_t k = 0; k < near.size() && k < 5; k++) {
                int b = near[k].second;
                if ((int)a < b || true) edges.push_back({ near[k].first, (int)a, b });
            }
        }
        std::sort(edges.begin(), edges.end(), [](const Edge& x, const Edge& y) { return x.d < y.d; });

        std::vector<int> uf(nodes.size());
        for (size_t i = 0; i < uf.size(); i++) uf[i] = (int)i;
        std::function<int(int)> find = [&](int x) { while (uf[x] != x) { uf[x] = uf[uf[x]]; x = uf[x]; } return x; };
        std::set<std::pair<int,int>> built;

        for (size_t e = 0; e < edges.size(); e++) {
            if (e % 64 == 0) g_progress.store(75 + (int)(18.f * e / std::max<size_t>(1, edges.size())));
            int a = edges[e].a, b = edges[e].b;
            std::pair<int,int> key(std::min(a, b), std::max(a, b));
            if (built.count(key)) continue;
            bool sameComp = find(a) == find(b);
            if (sameComp && !rng.chance(0.16f)) continue;          // a few extra loops for realism
            if (BuildRoad(sites[nodes[a]].x, sites[nodes[a]].y, sites[nodes[b]].x, sites[nodes[b]].y)) {
                built.insert(key);
                uf[find(a)] = find(b);
            }
        }
        // footpaths to remote places
        for (size_t w = 0; w < wild.size(); w++) {
            if (!rng.chance(0.45f)) continue;
            int best = -1; float bd = 1e9f;
            for (size_t b = 0; b < nodes.size(); b++) {
                float dx = (float)(sites[wild[w]].x - sites[nodes[b]].x), dy = (float)(sites[wild[w]].y - sites[nodes[b]].y);
                float d = sqrtf(dx * dx + dy * dy);
                if (d < bd) { bd = d; best = (int)b; }
            }
            if (best >= 0 && bd <= 10.f) BuildRoad(sites[wild[w]].x, sites[wild[w]].y, sites[nodes[best]].x, sites[nodes[best]].y);
        }
    }

    // ------------------------------------------------------------------ write into worldMap
    void Finish()
    {
        Stage(8, 95);
        std::set<std::string> used;
        for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
            int i = I(x, y);
            WorldTile* t = &map[y][x];
            memset(t, 0, sizeof(WorldTile));
            t->biome = biome[i];
            t->elevation = elev[i];
            t->temperature = temp[i];
            t->moisture = rain[i];
            t->riverMask = riverMask[i];
            t->riverWidth = riverWidth[i];
            t->roadMask = roadMask[i];
            t->isLake = lake[i];
            t->riverFlow = (unsigned short)std::min(65535.f, acc[i]);
            t->hasLocalMap = true;
            t->discovered = false;
            t->hasExitPoint = false;
            t->lastExitX = 128; t->lastExitY = 128;
            switch (biome[i]) {
                case BIOME_DEEP_OCEAN: case BIOME_OCEAN: case BIOME_LAKE: t->worldTile = '~'; break;
                case BIOME_FOREST: case BIOME_JUNGLE: case BIOME_TAIGA: t->worldTile = 'T'; break;
                case BIOME_MOUNTAINS: case BIOME_SNOW_PEAKS: t->worldTile = '^'; break;
                default: t->worldTile = '.'; break;
            }
            snprintf(t->locationName, sizeof(t->locationName), "%s", BiomeName(biome[i]));
        }
        for (size_t s = 0; s < sites.size(); s++) {
            int i = I(sites[s].x, sites[s].y);
            WorldTile* t = &map[sites[s].y][sites[s].x];
            t->locationType = sites[s].type;
            NameFlavor fl = NAME_PLAIN;
            if (nearOcean[i]) fl = NAME_COAST;
            else if (nearRiver[i]) fl = NAME_RIVER;
            else if (biome[i] == BIOME_HILLS || nearMtn[i]) fl = NAME_HILL;
            else if (biome[i] == BIOME_FOREST || biome[i] == BIOME_JUNGLE || biome[i] == BIOME_TAIGA) fl = NAME_FOREST;
            char name[50];
            for (int attempt = 0; attempt < 12; attempt++) {
                GenerateLocationName(name, sizeof(name), sites[s].type, fl, rng);
                if (!used.count(name)) break;
            }
            if (used.count(name)) {   // exhausted unique combos: add a numeral
                char tmp[50]; snprintf(tmp, sizeof(tmp), "%s II", name); snprintf(name, sizeof(name), "%s", tmp);
            }
            used.insert(name);
            snprintf(t->locationName, sizeof(t->locationName), "%s", name);
        }
        g_progress.store(100);
    }

    void Run(int* startX, int* startY)
    {
        GenElevation();
        GenClimate();
        GenHydrology();
        GenBiomes();
        GenSites(startX, startY);
        GenRoads();
        Finish();
    }
};

} // namespace

// ---------------------------------------------------------------------------
// public API
// ---------------------------------------------------------------------------
void WorldGen_Generate(int width, int height, unsigned int seed, WorldTile** map, int* startX, int* startY)
{
    Generator gen(width, height, seed, map);
    int sx = 0, sy = 0;
    gen.Run(&sx, &sy);
    if (startX) *startX = sx;
    if (startY) *startY = sy;
}

void WorldGen_StartAsync(int width, int height, unsigned int seed, WorldTile** map)
{
    WorldGen_Join();
    g_done.store(false);
    g_progress.store(0);
    g_stage.store(0);
    g_thread = new std::thread([=]() {
        int sx = 0, sy = 0;
        WorldGen_Generate(width, height, seed, map, &sx, &sy);
        g_startX = sx; g_startY = sy;
        g_done.store(true);
    });
}

bool WorldGen_IsDone() { return g_done.load(); }
int  WorldGen_GetProgress() { return g_progress.load(); }
const char* WorldGen_GetStageName() { int s = g_stage.load(); return STAGE_NAMES[s < 0 ? 0 : (s > 8 ? 8 : s)]; }
void WorldGen_Join() { if (g_thread) { if (g_thread->joinable()) g_thread->join(); delete g_thread; g_thread = NULL; } }
void WorldGen_GetStart(int* x, int* y) { if (x) *x = g_startX; if (y) *y = g_startY; }
