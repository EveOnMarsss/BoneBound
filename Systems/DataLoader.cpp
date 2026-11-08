#include "DataLoader.h"
#include "../ThirdParty/json.hpp"

#include <algorithm>   // std::sort
#include <filesystem>  // look inside folders
#include <fstream>     // open files
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;
namespace fs = std::filesystem;

// The one shared copy of the game data (the header only promised it exists, this creates it).
GameData gameData;

// ===========================================================================
// Warm-up: reading a plain text file
// ===========================================================================
std::vector<std::string> ReadLinesFromFile(const std::string& path)
{
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;   // the file closes itself when 'file' goes out of scope
}

bool LoadWordList(const std::string& path, std::vector<std::string>& words)
{
    try {
        words = ReadLinesFromFile(path);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading word list: " << e.what() << std::endl;
        words.clear();
        return false;
    }
}

// ===========================================================================
// Small helpers that read one value out of a JSON object.
// 'static' = only this file can use them. Each one checks the value is the right type,
// and on a problem sets 'error' to a message and returns false.
// ===========================================================================

// Opens a file and checks it holds one { ... } block. Used by every kind of data file.
static bool OpenJsonFile(const std::string& path, json& out, std::string& error)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        error = "could not open the file";
        return false;
    }
    // 'false' = do not crash on a typo, give us a "discarded" value instead
    out = json::parse(file, nullptr, false);
    if (out.is_discarded()) {
        error = "not valid JSON (check commas, quotes and brackets)";
        return false;
    }
    if (!out.is_object()) {
        error = "the file must contain one { ... } block";
        return false;
    }
    return true;
}

// required text, like  "id": "dog_kingdom"
static bool ReadString(const json& parent, const char* key, std::string& out, std::string& error)
{
    if (!parent.contains(key) || !parent[key].is_string()) {
        error = std::string("missing text value \"") + key + "\"";
        return false;
    }
    out = parent[key].get<std::string>();
    if (out.empty()) {
        error = std::string("\"") + key + "\" is empty";
        return false;
    }
    return true;
}

// optional text: if the key is missing 'out' is left alone (keeps its default)
static bool ReadOptionalString(const json& parent, const char* key, std::string& out, std::string& error)
{
    if (!parent.contains(key)) return true;
    if (!parent[key].is_string()) {
        error = std::string("\"") + key + "\" must be text";
        return false;
    }
    out = parent[key].get<std::string>();
    return true;
}

// optional whole number: if the key is missing 'out' is left alone (keeps its default, usually 0)
static bool ReadOptionalInt(const json& parent, const char* key, int& out, std::string& error)
{
    if (!parent.contains(key)) return true;
    if (!parent[key].is_number_integer()) {
        error = std::string("\"") + key + "\" must be a whole number";
        return false;
    }
    out = parent[key].get<int>();
    return true;
}

// a list of text like  "starts": ["Bark", "Woof", "Grr"]
static bool ReadStringList(const json& parent, const char* key, std::vector<std::string>& out, std::string& error)
{
    if (!parent.contains(key) || !parent[key].is_array()) {
        error = std::string("missing list \"") + key + "\"";
        return false;
    }
    out.clear();
    for (const auto& item : parent[key]) {
        if (!item.is_string()) {
            error = std::string("the list \"") + key + "\" must contain only text";
            return false;
        }
        out.push_back(item.get<std::string>());
    }
    if (out.empty()) {
        error = std::string("the list \"") + key + "\" is empty";
        return false;
    }
    return true;
}

// a block like  "settlement": { "starts": [...], "ends": [...] }
static bool ReadNameParts(const json& parent, const char* key, NameParts& out, std::string& error)
{
    if (!parent.contains(key) || !parent[key].is_object()) {
        error = std::string("missing section \"") + key + "\"";
        return false;
    }
    std::string problem;
    if (!ReadStringList(parent[key], "starts", out.starts, problem) ||
        !ReadStringList(parent[key], "ends", out.ends, problem)) {
        error = std::string("in section \"") + key + "\": " + problem;
        return false;
    }
    return true;
}

// ===========================================================================
// Cultures: ONE culture per file
// ===========================================================================
bool LoadCultureFile(const std::string& path, Culture& out, std::string& error)
{
    json data;
    if (!OpenJsonFile(path, data, error)) {
        return false;
    }

    // Fill a temporary Culture first. 'out' is only touched if EVERYTHING worked,
    // so a half-broken file can never leave half-filled data behind.
    Culture c;
    std::string problem;
    if (!ReadString(data, "id", c.id, problem) ||
        !ReadString(data, "species", c.species, problem) ||
        !ReadNameParts(data, "settlement", c.settlement, problem) ||
        !ReadNameParts(data, "people", c.people, problem)) {
        error = problem;
        return false;
    }

    out = c;
    return true;
}

// ===========================================================================
// Items, quests and dialogue: MANY entries per file.
// Each kind has one small "ParseX" function that turns ONE { ... } entry into a struct.
// ===========================================================================

static bool ParseItemType(const std::string& text, ItemType& out)
{
    if (text == "consumable") { out = ITEM_CONSUMABLE; return true; }
    if (text == "weapon")     { out = ITEM_WEAPON;     return true; }
    if (text == "quest")      { out = ITEM_QUEST;      return true; }
    return false;                                  // not one of the known words
}

static bool ParseItem(const json& entry, ItemDef& out, std::string& error)
{
    if (!entry.is_object()) {
        error = "each entry must be a { ... } block";
        return false;
    }
    ItemDef item;
    std::string typeText;
    if (!ReadString(entry, "id", item.id, error) ||
        !ReadString(entry, "name", item.name, error) ||
        !ReadString(entry, "type", typeText, error)) {
        return false;
    }
    if (!ParseItemType(typeText, item.type)) {
        error = "unknown item type \"" + typeText + "\" (use consumable, weapon or quest)";
        return false;
    }
    if (!ReadOptionalInt(entry, "heal", item.heal, error) ||
        !ReadOptionalInt(entry, "attack", item.attack, error) ||
        !ReadOptionalInt(entry, "value", item.value, error)) {
        return false;
    }
    out = item;
    return true;
}

static bool ParseQuestType(const std::string& text, QuestType& out)
{
    if (text == "fetch")   { out = QUEST_FETCH;   return true; }
    if (text == "kill")    { out = QUEST_KILL;    return true; }
    if (text == "reach")   { out = QUEST_REACH;   return true; }
    if (text == "deliver") { out = QUEST_DELIVER; return true; }
    return false;
}

static bool ParseQuest(const json& entry, QuestDef& out, std::string& error)
{
    if (!entry.is_object()) {
        error = "each entry must be a { ... } block";
        return false;
    }
    QuestDef quest;
    std::string typeText;
    if (!ReadString(entry, "id", quest.id, error) ||
        !ReadString(entry, "name", quest.name, error) ||
        !ReadString(entry, "type", typeText, error) ||
        !ReadString(entry, "description", quest.description, error)) {
        return false;
    }
    if (!ParseQuestType(typeText, quest.type)) {
        error = "unknown quest type \"" + typeText + "\" (use fetch, kill, reach or deliver)";
        return false;
    }
    if (!ReadOptionalString(entry, "item", quest.item, error) ||
        !ReadOptionalInt(entry, "reward_gold", quest.rewardGold, error)) {
        return false;
    }
    if (quest.type == QUEST_FETCH && quest.item.empty()) {
        error = "a fetch quest needs an \"item\"";
        return false;
    }
    out = quest;
    return true;
}

static bool ParseDialogueLine(const json& entry, DialogueLine& out, std::string& error)
{
    if (!entry.is_object()) {
        error = "each entry must be a { ... } block";
        return false;
    }
    DialogueLine line;
    if (!ReadString(entry, "id", line.id, error) ||
        !ReadString(entry, "category", line.category, error) ||
        !ReadString(entry, "text", line.text, error)) {
        return false;
    }
    // optional  "requires": { "role": "villager", "at_war": "true" }  (every value is text for now)
    if (entry.contains("requires")) {
        if (!entry["requires"].is_object()) {
            error = "\"requires\" must be a { ... } block";
            return false;
        }
        for (auto it = entry["requires"].begin(); it != entry["requires"].end(); ++it) {
            if (!it.value().is_string()) {
                error = "in \"requires\", the value of \"" + it.key() + "\" must be text";
                return false;
            }
            line.conditions[it.key()] = it.value().get<std::string>();
        }
    }
    out = line;
    return true;
}

// ===========================================================================
// Finding files and folders
// ===========================================================================

// Every ".json" file directly inside 'folder', sorted by name (so the order is the same on every computer).
std::vector<std::string> FindJsonFiles(const std::string& folder)
{
    std::vector<std::string> files;

    std::error_code ec;                        // lets filesystem report problems without throwing
    if (!fs::is_directory(folder, ec)) {
        return files;                          // a missing folder is normal: most mods won't have every kind of content
    }

    for (const auto& entry : fs::directory_iterator(folder, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path().generic_string());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

// The list of places to load data from: the base game first, then every mod folder.
// Later entries are loaded later, so a mod can replace something from the base game.
std::vector<std::string> GetDataRoots(const std::string& baseFolder, const std::string& modsFolder)
{
    std::vector<std::string> roots;
    roots.push_back(baseFolder);

    std::error_code ec;
    if (fs::is_directory(modsFolder, ec)) {
        std::vector<std::string> modRoots;
        for (const auto& entry : fs::directory_iterator(modsFolder, ec)) {
            if (entry.is_directory()) {
                modRoots.push_back(entry.path().generic_string() + "/data");
            }
        }
        std::sort(modRoots.begin(), modRoots.end());
        roots.insert(roots.end(), modRoots.begin(), modRoots.end());
    }
    return roots;
}

// ---------------------------------------------------------------------------
// ONE function that loads any "many entries per file" kind, so we do not copy the same loop three times.
//   'T' stands for "whichever struct": ItemDef, QuestDef or DialogueLine. The compiler fills it in.
//   folder    : which subfolder to read, like "items"
//   listKey   : the name of the list inside each file, like "items" or "lines"
//   parseEntry: the function that reads one entry (ParseItem, ParseQuest, ...)
//   target    : the map to put the results in
// ---------------------------------------------------------------------------
template <typename T>
static void LoadListFiles(const std::string& root, const char* folder, const char* listKey,
                          bool (*parseEntry)(const json&, T&, std::string&),
                          std::unordered_map<std::string, T>& target,
                          std::vector<std::string>& messages)
{
    for (const std::string& file : FindJsonFiles(root + "/" + folder)) {

        json data;
        std::string error;
        if (!OpenJsonFile(file, data, error)) {
            messages.push_back("ERROR " + file + ": " + error);
            continue;                          // skip this file, keep loading the rest
        }
        if (!data.contains(listKey) || !data[listKey].is_array()) {
            messages.push_back("ERROR " + file + ": missing the list \"" + listKey + "\"");
            continue;
        }

        int number = 0;
        for (const auto& entry : data[listKey]) {
            number++;
            T item;
            if (!parseEntry(entry, item, error)) {
                // one bad entry does NOT stop the good ones after it
                messages.push_back("ERROR " + file + " entry #" + std::to_string(number) + ": " + error);
                continue;
            }
            if (target.count(item.id) > 0) {
                messages.push_back("NOTE " + file + " replaces the " + folder + " entry \"" + item.id + "\"");
            }
            target[item.id] = item;            // a later file (a mod) wins
        }
    }
}

// ===========================================================================
// Loading everything
// ===========================================================================
void LoadGameData(GameData& data, const std::string& baseFolder, const std::string& modsFolder)
{
    data.cultures.clear();
    data.items.clear();
    data.quests.clear();
    data.dialogue.clear();
    data.messages.clear();

    for (const std::string& root : GetDataRoots(baseFolder, modsFolder)) {

        // ---- cultures (one per file) ----
        for (const std::string& file : FindJsonFiles(root + "/cultures")) {
            Culture culture;
            std::string error;
            if (!LoadCultureFile(file, culture, error)) {
                data.messages.push_back("ERROR " + file + ": " + error);
                continue;
            }

            // The id inside the file should match the file name ("cat_kingdom.json" -> "cat_kingdom")
            std::string expected = fs::path(file).stem().string();
            if (culture.id != expected) {
                data.messages.push_back("WARNING " + file + ": the id inside is \"" + culture.id +
                                        "\" but the file is named \"" + expected + "\"");
            }
            if (data.cultures.count(culture.id) > 0) {
                data.messages.push_back("NOTE " + file + " replaces the culture \"" + culture.id + "\"");
            }
            data.cultures[culture.id] = culture;
        }

        // ---- items, quests, dialogue (many per file) ----
        LoadListFiles(root, "items",    "items", ParseItem,         data.items,    data.messages);
        LoadListFiles(root, "quests",   "quests", ParseQuest,       data.quests,   data.messages);
        LoadListFiles(root, "dialogue", "lines", ParseDialogueLine, data.dialogue, data.messages);
    }

    // ---- cross-check: do the ids that one file mentions really exist in another? ----
    // (done after everything is loaded, so a mod's items count too)
    for (const auto& pair : data.quests) {
        const QuestDef& quest = pair.second;
        if (!quest.item.empty() && data.items.count(quest.item) == 0) {
            data.messages.push_back("WARNING quest \"" + quest.id + "\" needs the item \"" + quest.item +
                                    "\", but no item has that id");
        }
    }

    data.messages.push_back("Loaded " + std::to_string(data.cultures.size()) + " culture(s), " +
                            std::to_string(data.items.size()) + " item(s), " +
                            std::to_string(data.quests.size()) + " quest(s), " +
                            std::to_string(data.dialogue.size()) + " dialogue line(s)");
}

// ===========================================================================
// Looking something up. Each returns nullptr when the id is unknown.
// ===========================================================================
const Culture* FindCulture(const GameData& data, const std::string& id)
{
    auto it = data.cultures.find(id);          // 'it' points at the match, or at end() if there is none
    if (it == data.cultures.end()) {
        return nullptr;                        // not found
    }
    return &it->second;                        // it->first is the id, it->second is the Culture
}

const ItemDef* FindItem(const GameData& data, const std::string& id)
{
    auto it = data.items.find(id);
    if (it == data.items.end()) return nullptr;
    return &it->second;
}

const QuestDef* FindQuest(const GameData& data, const std::string& id)
{
    auto it = data.quests.find(id);
    if (it == data.quests.end()) return nullptr;
    return &it->second;
}