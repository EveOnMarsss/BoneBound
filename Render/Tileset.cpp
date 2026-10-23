#include "Tileset.h"

static Texture2D g_tex = { 0 };
static bool g_ready = false;

void TilesetInit(const char* path)
{
    Image img = LoadImage(path);
    if (!img.data) {
        LOG("WARNING: tileset '%s' not found - falling back to text glyphs\n", path);
        g_ready = false;
        return;
    }
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* px = (Color*)img.data;
    const int n = img.width * img.height;
    for (int i = 0; i < n; i++) {
        // glyph pixels are white/grey, the background is magenta (green channel = 0)
        unsigned char a = px[i].g;
        px[i].r = 255; px[i].g = 255; px[i].b = 255; px[i].a = a;
    }
    g_tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(g_tex, TEXTURE_FILTER_POINT);
    g_ready = (g_tex.id != 0);
    LOG("Tileset loaded: %s (%dx%d)\n", path, g_tex.width, g_tex.height);
}

void TilesetShutdown()
{
    if (g_ready) UnloadTexture(g_tex);
    g_ready = false;
}

bool TilesetReady() { return g_ready; }

void DrawGlyphAt(int glyph, float x, float y, float size, Color fg)
{
    if (g_ready) {
        Rectangle src = { (float)((glyph & 15) * TILESET_CELL), (float)((glyph >> 4) * TILESET_CELL),
                          (float)TILESET_CELL, (float)TILESET_CELL };
        Rectangle dst = { x, y, size, size };
        DrawTexturePro(g_tex, src, dst, (Vector2){ 0, 0 }, 0.f, fg);
    } else {
        char ch = (glyph >= 32 && glyph < 127) ? (char)glyph : '#';
        DrawTextEx(GetFontDefault(), TextFormat("%c", ch), (Vector2){ x + size * 0.25f, y + size * 0.1f }, size * 0.8f, 1, fg);
    }
}

void DrawGlyphCell(int glyph, int tx, int ty, Color fg, Color bg)
{
    float x = (float)(tx * TILE_SIZE), y = (float)(ty * TILE_SIZE);
    if (bg.a) DrawRectangle((int)x, (int)y, TILE_SIZE, TILE_SIZE, bg);
    DrawGlyphAt(glyph, x, y, (float)TILE_SIZE, fg);
}
