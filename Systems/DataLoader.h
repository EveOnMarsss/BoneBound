#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// The SHAPES of the data. Each struct mirrors one { ... } block in a JSON file.
// ---------------------------------------------------------------------------

// ---- cultures (one file per culture) ----
struct NameParts
{
    std::vector<std::string> starts;   // "Bark", "Woof", ...
    std::vector<std::string> ends;     // "ton", "ville", ...
};

struct Culture
{
    std::string id;         // "dog_kingdom"
    std::string species;    // "dog"
    NameParts settlement;   // pieces for naming villages
    NameParts people;       // pieces for naming people
};

// ---- items (many entries per file) ----
enum ItemType { ITEM_CONSUMABLE, ITEM_WEAPON, ITEM_QUEST };

struct ItemDef
{
    std::string id;                      // "health_potion"
    std::string name;                    // "Health Potion"
    ItemType type = ITEM_CONSUMABLE;
    int heal = 0;                        // 0 when the file does not mention it
    int attack = 0;
    int value = 0;                       // price
};

// ---- quests (many entries per file) ----
enum QuestType { QUEST_FETCH, QUEST_KILL, QUEST_REACH, QUEST_DELIVER };

struct QuestDef
{
    std::string id;                      // "fetch_lost_sword"
    std::string name;                    // "Find the Lost Sword"
    QuestType type = QUEST_FETCH;
    std::string item;                    // id of an item in items.json ("" = none)
    std::string description;             // may contain slots like {place}
    int rewardGold = 0;
};

// ---- dialogue (many entries per file) ----
struct DialogueLine
{
    std::string id;                                          // "greet_villager_welcome"
    std::string category;                                    // "greeting"
    std::unordered_map<std::string, std::string> conditions; // from "requires": e.g. role -> villager
    std::string text;
};

// Everything the game loads from the data folders lives in one place.
struct GameData
{
    std::unordered_map<std::string, Culture>      cultures;   // each map is looked up by id
    std::unordered_map<std::string, ItemDef>      items;
    std::unordered_map<std::string, QuestDef>     quests;
    std::unordered_map<std::string, DialogueLine> dialogue;
    std::vector<std::string> messages;                        // errors, warnings and notes found while loading
};

// The one shared copy the game uses. Declared here with 'extern', created once in DataLoader.cpp.
extern GameData gameData;

// Loads every data file from the base game, then from every mod. Call it once at startup.
void LoadGameData(GameData& data,
                  const std::string& baseFolder = "data/base",
                  const std::string& modsFolder = "mods");

// Lookups. They return nullptr when there is no such id - always check for that!
const Culture* FindCulture(const GameData& data, const std::string& id);
const ItemDef* FindItem(const GameData& data, const std::string& id);
const QuestDef* FindQuest(const GameData& data, const std::string& id);

// Building blocks. They are public so a test program can try them one by one.
std::vector<std::string> GetDataRoots(const std::string& baseFolder = "data/base",
                                      const std::string& modsFolder = "mods");
std::vector<std::string> FindJsonFiles(const std::string& folder);
bool LoadCultureFile(const std::string& path, Culture& out, std::string& error);

// The plain-text warm-up functions from before. Kept so your old tests still work.
std::vector<std::string> ReadLinesFromFile(const std::string& path);
bool LoadWordList(const std::string& path, std::vector<std::string>& words);

#endif