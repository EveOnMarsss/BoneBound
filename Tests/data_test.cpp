#include "../Systems/DataLoader.h"
#include <cstdio>

int main()
{
    LoadGameData(gameData);
    for (const std::string& message : gameData.messages)
    {
        printf("%s\n", message.c_str());
    }

    const ItemDef* sword = FindItem(gameData, "iron_sword");
    if (sword == nullptr) { printf("iron_sword NOT FOUND\n"); return 1; }
    printf("%s: attack %d, costs %d\n", sword->name.c_str(), sword->attack, sword->value);

    const QuestDef* quest = FindQuest(gameData, "fetch_lost_sword");
    if (quest != nullptr) printf("quest: %s (reward %d gold)\n", quest->name.c_str(), quest->rewardGold);
    return 0;
}