#ifndef TILESET_H
#define TILESET_H

#include "../project.h"

// CP437 tileset renderer (the 12x12 "DB curses" sheet: 16x16 glyphs, magenta = transparent).
// All glyphs are drawn with DrawTexturePro using the same texture, so raylib batches them into
// a handful of draw calls (much faster than switching shaders for every glyph).

void TilesetInit(const char* path);
void TilesetShutdown();
bool TilesetReady();

// Draw one glyph in a TILE_SIZE cell at tile coordinates (tx, ty). bg.a == 0 means no background.
void DrawGlyphCell(int glyph, int tx, int ty, Color fg, Color bg);

// Draw one glyph at an arbitrary world-space pixel position/size.
void DrawGlyphAt(int glyph, float x, float y, float size, Color fg);

#endif
