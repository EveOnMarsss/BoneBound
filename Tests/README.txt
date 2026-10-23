Headless tests (no window needed) - they stub the few raylib calls the game logic uses.

game_test.cpp   Generates worlds of every size, builds local maps for every location type, checks determinism,
                seamless tile travel, save/load round trips (also mid-map) and rejection of bad saves.
                Build with AddressSanitizer from the project root:
                g++ -std=c++17 -g -fsanitize=address -I. -I<raylib>/src Tests/game_test.cpp Globals.cpp \
                  Entities/SpawnPoint.cpp Entities/NPC.cpp World/LocalMap.cpp World/WorldMap.cpp World/Tiles.cpp \
                  World/WorldGen.cpp World/LocalTerrain.cpp World/LocationNames.cpp Systems/SaveLoad.cpp \
                  Game/Camera.cpp -lpthread -Wl,--unresolved-symbols=ignore-all
render_test.cpp + draw.py   Render world / local-map views to PNG using resources/tileset (needs Python + Pillow).
wg_test.cpp     World generator statistics + biome image.
