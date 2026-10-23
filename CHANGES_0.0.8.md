# BoneBound 0.0.8

## Bugs fixed
1. **Map size always MEDIUM** - the character screen reused `selectedOption`; the size is now kept in `chosenMapSize`.
2. **ESC quit the game** - `SetExitKey(KEY_NULL)`; ESC now opens a **pause menu** (Resume / Save / Load / Quit to Title / Quit).
3. **Typing "f" toggled fullscreen** - fullscreen is **F11** now.
4. **Walk-through furniture / water** - one tile table (`World/Tiles.cpp`) now defines glyph, colour, walkable, buildable.
   `T` is a table; trees are `t` (they used to share a letter).
5. **Save/Load** rewritten (version header, full character, world data, atomic load, rejects damaged/old files).
   No more uninitialised `buildings`/tile fields. Local maps are not saved - they regenerate from the seed.
6. **Logger** - one shared log file handle (it used to be truncated by every .cpp file).
7. **Backspace on world map dumped you to the title** - removed; Save/Load menus return to where they were opened from.
Also: key-repeat movement, NPCs/player no longer overlap, unique NPC ids, spawn counters reset on re-entry, camera
no longer updated twice per frame, conversation text word-wraps, player is `@`, version string defined once.

## World generation (World/WorldGen.cpp) - pure C++, seeded, threaded with a loading screen
continents (domain-warped noise, fixed land %) -> mountain ranges -> latitude/altitude temperature ->
wind-driven rainfall with ocean moisture and rain shadows -> priority-flood hydrology (lakes, rivers that always
reach the sea) -> Whittaker-style biomes -> geography-aware settlement siting (rivers, coasts, hills) ->
road network (A*, reuses existing roads, bridges rivers) -> place names that reflect the terrain.

## Local maps (World/LocalTerrain.cpp)
Terrain is sampled from continuous world fields in global coordinates, so **neighbouring tiles line up**:
coastlines, biome borders, rivers (with fords) and roads (with bridges) continue across tile edges.
You can now **walk off the edge of a map into the next one**. Forests/mountains get natural trails and a
connectivity pass so nobody spawns walled in. Settlements get a cleared, flattened area.

## Rendering
CP437 tileset (resources/tileset) via batched glyph draws; world map shows biomes, rivers, roads, relief, and
switches to colour blocks when zoomed far out.

## Notes
* New source files must be part of your build (Render/, World/Tiles.cpp, WorldGen.cpp, LocalTerrain.cpp, LocationNames.cpp).
  WorldGen uses std::thread (needs the posix-thread MinGW, as your Raylib Testing project already did).
* World::FastNoiseLite.h is MIT licensed (Jordan Peck) - keep its header.
* Old v1 saves are not compatible (they never stored enough to restore a game).

# BoneBound 0.0.9

* **Choose your starting location**: after the world is generated you pick where to begin (new state `STATE_CHOOSE_START`,
  code in `UI/Menus.cpp`: `StartPickUpdate` / `StartPickDraw`). The whole map is revealed while choosing, the cursor starts on a
  recommended village, open water is rejected. Arrows/WASD move (SHIFT = 5 tiles), Wheel/Z zoom, R = recommended start,
  ENTER = begin, ESC = discard this world.
* **World sizes doubled**: TINY 16, SMALL 32, MEDIUM 64, LARGE 128, HUGE 256, GIGANTIC 512 (generates in ~0.35 s; a 512 save is ~19 MB).
  Lake and river thresholds now scale with world size so big worlds don't drown in puddles.
* **Fullscreen option** in the title menu and the pause menu (F11 still works).
* Fixed the NPC narrowing warning.
