#include "NPC.h"
#include "../Render/Tileset.h"
#include "SpawnPoint.h"
#include "../World/LocalMap.h"
#include "../Game/Camera.h"
#include "../UI/Conversation.h"
#include <time.h>

// ===== NAME DATABASE DEFINITIONS =====
const char* FIRST_NAMES[] = {
    // Male
    "John", "William", "James", "Robert", "Michael", "Thomas", "Richard", "Charles",
    "Henry", "Edward", "George", "Andrew", "David", "Daniel", "Matthew", "Christopher",
    "Anthony", "Donald", "Mark", "Paul", "Steven", "Kenneth", "Joshua", "Kevin",
    "Brian", "Timothy", "Ronald", "Jason", "Jeffrey", "Ryan", "Jacob", "Gary",
    "Nicholas", "Eric", "Stephen", "Jonathan", "Larry", "Justin", "Scott", "Brandon",
    "Benjamin", "Samuel", "Gregory", "Frank", "Alexander", "Raymond", "Patrick", "Jack",
    "Dennis", "Jerry", "Tyler", "Aaron", "Jose", "Adam", "Nathan", "Peter", "Zachary",
    "Walter", "Harold", "Douglas", "Carl", "Arthur", "Roger", "Keith", "Gerald",
    "Lawrence", "Terry", "Sean", "Austin", "Christian", "Jesse", "Ethan", "Billy",
    "Bruce", "Ralph", "Roy", "Jordan", "Bryan", "Alan", "Wayne", "Eugene", "Logan",
    "Randy", "Louis", "Russell", "Vincent", "Philip", "Bobby", "Johnny", "Bradley",
    
    // Female
    "Mary", "Patricia", "Jennifer", "Linda", "Elizabeth", "Barbara", "Susan", "Jessica",
    "Sarah", "Karen", "Nancy", "Lisa", "Betty", "Margaret", "Sandra", "Ashley",
    "Kimberly", "Emily", "Donna", "Michelle", "Dorothy", "Carol", "Amanda", "Melissa",
    "Deborah", "Stephanie", "Rebecca", "Sharon", "Laura", "Cynthia", "Kathleen", "Amy",
    "Shirley", "Angela", "Helen", "Anna", "Brenda", "Pamela", "Nicole", "Samantha",
    "Katherine", "Emma", "Ruth", "Christine", "Catherine", "Debra", "Rachel", "Carolyn",
    "Janet", "Virginia", "Maria", "Heather", "Diane", "Julie", "Joyce", "Victoria",
    "Kelly", "Christina", "Lauren", "Joan", "Evelyn", "Olivia", "Judith", "Megan",
    "Cheryl", "Martha", "Andrea", "Frances", "Hannah", "Jacqueline", "Ann", "Gloria",
    "Jean", "Kathryn", "Alice", "Teresa", "Sara", "Janice", "Doris", "Madison",
    "Julia", "Grace", "Judy", "Abigail", "Marie", "Denise", "Beverly", "Amber",
    
    // Fantasy/Medieval
    "Aric", "Brom", "Cedric", "Dorian", "Eldric", "Finnian", "Gareth", "Hadrian",
    "Ivor", "Jorah", "Kaelen", "Leofric", "Merek", "Nolan", "Orin", "Percival",
    "Quinn", "Roland", "Sylas", "Theron", "Urien", "Valen", "Warrick", "Xander",
    "Yorick", "Zane", "Aelwen", "Briar", "Caelia", "Dwyn", "Elara", "Faelan",
    "Gwyneth", "Hestia", "Isolde", "Juniper", "Kaelia", "Lirien", "Morwen", "Nimue",
    "Oriana", "Phaedra", "Quenlya", "Rhiannon", "Seraphina", "Talia", "Una", "Vivienne",
    "Wynne", "Xanthe", "Ysmay", "Zephyra"
};

const char* LAST_NAMES[] = {
    // Common
    "Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller", "Davis",
    "Rodriguez", "Martinez", "Hernandez", "Lopez", "Gonzalez", "Wilson", "Anderson",
    "Thomas", "Taylor", "Moore", "Jackson", "Martin", "Lee", "Perez", "Thompson",
    "White", "Harris", "Sanchez", "Clark", "Ramirez", "Lewis", "Robinson", "Walker",
    "Young", "Allen", "King", "Wright", "Scott", "Torres", "Nguyen", "Hill", "Flores",
    "Green", "Adams", "Nelson", "Baker", "Hall", "Rivera", "Campbell", "Mitchell",
    "Carter", "Roberts", "Gomez", "Phillips", "Evans", "Turner", "Diaz", "Parker",
    "Cruz", "Edwards", "Collins", "Reyes", "Stewart", "Morris", "Morales", "Murphy",
    "Cook", "Rogers", "Gutierrez", "Ortiz", "Morgan", "Cooper", "Peterson", "Bailey",
    "Reed", "Kelly", "Howard", "Ramos", "Kim", "Cox", "Ward", "Richardson", "Watson",
    "Brooks", "Chavez", "Wood", "James", "Bennett", "Gray", "Mendoza", "Ruiz",
    "Hughes", "Price", "Alvarez", "Castillo", "Sanders", "Patel", "Myers", "Long",
    
    // Medieval/Occupation
    "Baker", "Butcher", "Carpenter", "Cooper", "Fletcher", "Gardner", "Hunter",
    "Mason", "Miller", "Miner", "Potter", "Sawyer", "Shepherd", "Smith", "Taylor",
    "Thatcher", "Weaver", "Wright", "Archer", "Bowman", "Carter", "Chandler",
    "Clark", "Cook", "Fisher", "Forester", "Fuller", "Glover", "Harper", "Hawkins",
    "Hayward", "Head", "Herd", "Hunt", "Kemp", "Knight", "Marshall", "Page",
    "Parker", "Parson", "Porter", "Reeve", "Saddler", "Sargeant", "Sexton",
    "Sherman", "Spencer", "Steward", "Stoddard", "Summers", "Ward", "Webb",
    
    // Location-based
    "Hill", "Lake", "Wood", "Forest", "Field", "Marsh", "Moore", "Brooks",
    "Rivers", "Stone", "Bridge", "Ford", "Haven", "Shore", "Creek", "Valley",
    "Meadows", "Grove", "Thorne", "Wells", "Springs", "Underwood", "Atwood",
    "Blackwood", "Greenwood", "Holbrook", "Linwood", "Oakley", "Prescott",
    "Radcliff", "Redmond", "Sherwood", "Stanley", "Sutton", "Thornton",
    "Wesley", "Weston", "Whitman", "Woodward", "Worth",
    
    // Fantasy
    "Ironheart", "Stormborn", "Winterfell", "Blackthorn", "Silverwood", "Goldmane",
    "Oakenshield", "Moonshadow", "Dragonborn", "Starweaver", "Frostbeard",
    "Lightfoot", "Darkwater", "Swiftarrow", "Strongbow", "Fairwind", "Ravenclaw",
    "Griffin", "Phoenix", "Stormcrow", "Whitewolf", "Redhand", "Graywolf",
    "Blackraven", "Silverstream", "Goldenleaf", "Copperfield", "Ironwood",
    "Steelheart", "Bronzebeard", "Silverblade", "Goldfinder", "Stonehelm",
    "Ironfist", "Strongarm", "Swiftwind", "Farsight", "Dawnbringer"
};

const int FIRST_NAME_COUNT = sizeof(FIRST_NAMES) / sizeof(FIRST_NAMES[0]);
const int LAST_NAME_COUNT = sizeof(LAST_NAMES) / sizeof(LAST_NAMES[0]);

// Generate a random name for NPCs
void GenerateNPCName(char* buffer, int bufferSize, bool isUnique)
{
    if (isUnique) {
        // Unique NPCs get a first and last name
        const char* firstName = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
        const char* lastName = LAST_NAMES[rand() % LAST_NAME_COUNT];
        snprintf(buffer, bufferSize, "%s %s", firstName, lastName);
    } else {
        // Generic NPCs just get a first name
        const char* firstName = FIRST_NAMES[rand() % FIRST_NAME_COUNT];
        snprintf(buffer, bufferSize, "%s", firstName);
    }
}

// Get color based on NPC role
Color GetRoleColor(NPCType type)
{
    switch (type) {
        case NPC_VILLAGER:
            // Brown shades for villagers
            switch(rand() % 6) {
                case 0: return (Color){ 139, 69, 19, 255 }; // Brown
                case 1: return (Color){ 160, 82, 45, 255 }; // Sienna
                case 2: return (Color){ 205, 133, 63, 255 }; // Peru
                case 3: return (Color){ 210, 105, 30, 255 }; // Chocolate
                case 4: return (Color){ 184, 115, 51, 255 }; // Chestnut
                default: return (Color){ 222, 184, 135, 255 }; // Burlywood
            }
            
        case NPC_GUARD:
            // Blue shades for guards
            return (Color){ 0, (unsigned char)(100 + (rand() % 100)), 200, 255 };
            
        case NPC_MERCHANT:
            // Green shades for merchants
            return (Color){ 0, (unsigned char)(100 + (rand() % 100)), 0, 255 };
            
        case NPC_INN_KEEPER:
            // Gold shades for innkeepers
            return (Color){ 255, (unsigned char)(200 + (rand() % 55)), 0, 255 };
            
        case NPC_BLACKSMITH:
            // Red/Orange shades for blacksmiths
            return (Color){ 200, (unsigned char)(50 + (rand() % 100)), 0, 255 };
            
        case NPC_WANDERER:
            // Gray shades for wanderers
            return (Color){ 
                (unsigned char)(150 + (rand() % 105)), 
                (unsigned char)(150 + (rand() % 105)), 
                (unsigned char)(150 + (rand() % 105)), 
                255 
            };
            
        case NPC_BANDIT:
            // Dark red/gray for bandits
            return (Color){ 
                (unsigned char)(100 + (rand() % 55)), 
                (unsigned char)(30 + (rand() % 30)), 
                (unsigned char)(30 + (rand() % 30)), 
                255 
            };
            
        case NPC_WIZARD:
            // Purple shades for wizards
            return (Color){ 
                (unsigned char)(150 + (rand() % 105)), 
                (unsigned char)(50 + (rand() % 50)), 
                (unsigned char)(200 + (rand() % 55)), 
                255 
            };
            
        default:
            return WHITE;
    }
}

// Add title based on NPC type for unique NPCs
void AddTitleToName(char* name, int bufferSize, NPCType type)
{
    char tempName[100];
    strcpy(tempName, name);
    
    switch (type) {
        case NPC_MERCHANT:
            snprintf(name, bufferSize, "%s the Merchant", tempName);
            break;
        case NPC_BLACKSMITH:
            snprintf(name, bufferSize, "%s the Blacksmith", tempName);
            break;
        case NPC_INN_KEEPER:
            snprintf(name, bufferSize, "%s the Innkeeper", tempName);
            break;
        case NPC_GUARD:
            snprintf(name, bufferSize, "%s the Guard", tempName);
            break;
        case NPC_WIZARD:
            snprintf(name, bufferSize, "%s the Wizard", tempName);
            break;
        case NPC_VILLAGER:
            // Just use name as is
            break;
        default:
            break;
    }
}

// Create NPC from spawn point with random positioning
NPC* CreateNPCFromSpawn(SpawnPoint* spawn, int worldX, int worldY)
{
    if (!spawn) {
        printf("ERROR: spawn is NULL in CreateNPCFromSpawn\n");
        return NULL;
    }
    
    NPC* npc = (NPC*)calloc(1, sizeof(NPC));
    if (!npc) return NULL;
    
    // Basic info
    static int nextNpcId = 1;          // npcCount is reused after NPCs are removed, which produced duplicate ids
    npc->id = nextNpcId++;
    npc->worldX = worldX;
    npc->worldY = worldY;
    npc->isInLocalMap = true;
    npc->isActive = true;
    npc->spawnPoint = spawn;
    npc->isUnique = spawn->isUnique;
    npc->type = spawn->npcType;
    
    // Position
    npc->localX = spawn->x;
    npc->localY = spawn->y;
    
    // Clamp to safe bounds
    if (npc->localX < 1) npc->localX = 1;
    if (npc->localY < 1) npc->localY = 1;
    if (npc->localX > LOCAL_MAP_WIDTH - 2) npc->localX = LOCAL_MAP_WIDTH - 2;
    if (npc->localY > LOCAL_MAP_HEIGHT - 2) npc->localY = LOCAL_MAP_HEIGHT - 2;
    
    // Set race (random for now, but you can make this deterministic based on NPC type or location)
    npc->isCat = (rand() % 2 == 0);
    npc->isDog = !npc->isCat;
    
    // Set display character based on race (C for Cat, D for Dog)
    npc->displayChar = npc->isCat ? 'c' : 'd';      // lowercase: uppercase letters are used by buildings / world map
    
    // Set color based on role
    npc->color = GetRoleColor(npc->type);
    
    // Use spawn name if provided, otherwise generate one
    if (spawn->npcName[0] != '\0') {
        strcpy(npc->name, spawn->npcName);
    } else {
        GenerateNPCName(npc->name, 50, spawn->isUnique);
    }
    
    // Add title for unique NPCs if desired (optional - comment out if you don't want titles)
    if (npc->isUnique && strchr(npc->name, ' ') != NULL) {
        // They already have a last name, add title
        AddTitleToName(npc->name, 50, npc->type);
    }
    
    // Conversation defaults
    npc->isTalking = false;
    npc->dialogState = DIALOG_GREETING;
    npc->selectedOption = 0;
    npc->chatLineCount = 0;
    npc->hasQuest = false;
    npc->canTrade = false;
    npc->canTalk = true;
    
    // Farewell default
    strcpy(npc->farewell, "Goodbye, traveler.");
    
    // Set NPC properties based on type
    switch (npc->type) {
        case NPC_VILLAGER:
            npc->isHostile = false;
            npc->health = 10 + (rand() % 10);
            npc->maxHealth = npc->health;
            npc->gold = rand() % 30;
            
            // Greetings
            {
                const char* greetings[] = {
                    "Hello there!",
                    "Lovely day, isn't it?",
                    "Have you seen my cow?",
                    "The harvest is good this year.",
                    "Welcome to our village!",
                    "Stay away from those bandits!",
                    "I'm just tending my garden.",
                    "The inn has the best ale.",
                    "Nice weather we're having."
                };
                strcpy(npc->greeting, greetings[rand() % 9]);
            }
            
            // Chat lines
            npc->chatLineCount = 4;
            strcpy(npc->chatLines[0], "The village has been peaceful lately.");
            strcpy(npc->chatLines[1], "My family has lived here for generations.");
            strcpy(npc->chatLines[2], "Have you tried the bread at the shop?");
            strcpy(npc->chatLines[3], "The guards keep us safe from bandits.");
            
            // Quests (10% chance)
            npc->hasQuest = (rand() % 10 == 0);
            if (npc->hasQuest) {
                const char* quests[] = {
                    "My cat is stuck in a tree! Can you help?",
                    "I lost my favorite ring in the forest.",
                    "Could you fetch some herbs for my sick mother?",
                    "The well needs cleaning. I'll pay you for the job.",
                    "My sheep have wandered off. Can you find them?"
                };
                strcpy(npc->questText, quests[rand() % 5]);
            }
            
            strcpy(npc->farewell, "Take care, friend!");
            break;
            
        case NPC_GUARD:
            npc->isHostile = false;
            npc->health = 25 + (rand() % 15);
            npc->maxHealth = npc->health;
            npc->gold = rand() % 20;
            
            // Greetings
            {
                const char* greetings[] = {
                    "Move along, citizen.",
                    "All quiet here.",
                    "Halt! Who goes there?",
                    "Just doing my duty.",
                    "Nothing to see here.",
                    "Keep the peace!",
                    "Any trouble?",
                    "The captain's in a mood today."
                };
                strcpy(npc->greeting, greetings[rand() % 8]);
            }
            
            // Chat lines
            npc->chatLineCount = 4;
            strcpy(npc->chatLines[0], "I've been on guard duty for hours.");
            strcpy(npc->chatLines[1], "Watch out for pickpockets in the market.");
            strcpy(npc->chatLines[2], "The prison cells are empty today.");
            strcpy(npc->chatLines[3], "I used to be an adventurer like you.");
            
            // Quests (15% chance)
            npc->hasQuest = (rand() % 15 == 0);
            if (npc->hasQuest) {
                const char* quests[] = {
                    "Bandits have been spotted near the old bridge.",
                    "A thief stole the mayor's necklace! Find them!",
                    "We need help patrolling the east road.",
                    "Someone's been poisoning the guard dogs.",
                    "There's a bounty on the goblin leader."
                };
                strcpy(npc->questText, quests[rand() % 5]);
            }
            
            strcpy(npc->farewell, "Stay safe out there.");
            break;
            
        case NPC_MERCHANT:
            npc->isHostile = false;
            npc->canTrade = true;
            npc->health = 12 + (rand() % 8);
            npc->maxHealth = npc->health;
            npc->gold = 50 + (rand() % 450);
            
            // Greetings
            {
                const char* greetings[] = {
                    "Care to browse my wares?",
                    "Best prices in town!",
                    "Fresh goods just arrived!",
                    "I have rare items today.",
                    "Need any supplies?",
                    "Special deal for you!",
                    "Take a look around.",
                    "Quality merchandise!"
                };
                strcpy(npc->greeting, greetings[rand() % 8]);
            }
            
            // Chat lines
            npc->chatLineCount = 4;
            strcpy(npc->chatLines[0], "Business has been good this season.");
            strcpy(npc->chatLines[1], "I source my goods from all over the region.");
            strcpy(npc->chatLines[2], "The trade routes have been dangerous lately.");
            strcpy(npc->chatLines[3], "I give discounts to regular customers.");
            
            // Quests (12% chance)
            npc->hasQuest = (rand() % 12 == 0);
            if (npc->hasQuest) {
                const char* quests[] = {
                    "My caravan was attacked! Can you recover my goods?",
                    "I need rare gems from the mountain caves.",
                    "A competitor is spreading lies about me.",
                    "I'm looking for a lost shipment of spices.",
                    "Someone stole my best merchandise!"
                };
                strcpy(npc->questText, quests[rand() % 5]);
            }
            
            strcpy(npc->tradeText, "Take a look at my fine wares!");
            strcpy(npc->farewell, "Come back soon!");
            break;
            
        case NPC_INN_KEEPER:
            npc->isHostile = false;
            npc->canTrade = true; // Sells rooms and food
            npc->health = 12 + (rand() % 8);
            npc->maxHealth = npc->health;
            npc->gold = 30 + (rand() % 100);
            
            // Greetings
            {
                const char* greetings[] = {
                    "Welcome to my inn!",
                    "Need a room for the night?",
                    "Best ale in the realm!",
                    "Make yourself at home.",
                    "The stew is excellent today.",
                    "Rooms are 10 gold a night.",
                    "Pull up a chair!",
                    "Warm fire and cold drinks!"
                };
                strcpy(npc->greeting, greetings[rand() % 8]);
            }
            
            // Chat lines
            npc->chatLineCount = 4;
            strcpy(npc->chatLines[0], "We get travelers from all over.");
            strcpy(npc->chatLines[1], "The stew recipe has been in my family for years.");
            strcpy(npc->chatLines[2], "I hear the roads are dangerous at night.");
            strcpy(npc->chatLines[3], "The mayor drinks here every evening.");
            
            // Quests (10% chance)
            npc->hasQuest = (rand() % 10 == 0);
            if (npc->hasQuest) {
                const char* quests[] = {
                    "My ale supplier hasn't arrived. Can you check on them?",
                    "A rowdy customer broke my best chair!",
                    "I need rare ingredients for my special stew.",
                    "Someone's been stealing from the kitchen.",
                    "The well behind the inn has gone dry."
                };
                strcpy(npc->questText, quests[rand() % 5]);
            }
            
            strcpy(npc->tradeText, "Would you like a room or a meal?");
            strcpy(npc->farewell, "Sleep well, traveler!");
            break;
            
        case NPC_BLACKSMITH:
            npc->isHostile = false;
            npc->canTrade = true;
            npc->health = 18 + (rand() % 12);
            npc->maxHealth = npc->health;
            npc->gold = 40 + (rand() % 160);
            
            // Greetings
            {
                const char* greetings[] = {
                    "Need something forged?",
                    "I can repair your gear.",
                    "Strongest swords in town!",
                    "Hot iron and hard work!",
                    "What can I make for you?",
                    "Special armor today.",
                    "Anvil's still hot!",
                    "Quality craftsmanship!"
                };
                strcpy(npc->greeting, greetings[rand() % 8]);
            }
            
            // Chat lines
            npc->chatLineCount = 4;
            strcpy(npc->chatLines[0], "I've been a smith for 30 years.");
            strcpy(npc->chatLines[1], "The best ore comes from the northern mines.");
            strcpy(npc->chatLines[2], "My father taught me everything I know.");
            strcpy(npc->chatLines[3], "A good sword takes time and care.");
            
            // Quests (15% chance)
            npc->hasQuest = (rand() % 15 == 0);
            if (npc->hasQuest) {
                const char* quests[] = {
                    "I need iron ore from the goblin caves.",
                    "Someone stole my finest sword!",
                    "The charcoal burner hasn't delivered.",
                    "I need a special alloy from the mountain.",
                    "My hammer broke! Can you find a replacement?"
                };
                strcpy(npc->questText, quests[rand() % 5]);
            }
            
            strcpy(npc->tradeText, "Browse my weapons and armor!");
            strcpy(npc->farewell, "Come back if you need sharpening!");
            break;
            
        case NPC_WANDERER:
            npc->isHostile = false;
            npc->health = 10 + (rand() % 15);
            npc->maxHealth = npc->health;
            npc->gold = rand() % 40;
            
            // Greetings
            {
                const char* greetings[] = {
                    "Greetings, traveler.",
                    "Long road ahead?",
                    "I've seen many lands.",
                    "The roads are dangerous.",
                    "Have you heard the news?",
                    "Rest while you can.",
                    "Where are you headed?",
                    "The world is vast."
                };
                strcpy(npc->greeting, greetings[rand() % 8]);
            }
            
            // Chat lines
            npc->chatLineCount = 4;
            strcpy(npc->chatLines[0], "I've walked from the distant mountains.");
            strcpy(npc->chatLines[1], "The cities in the east are magnificent.");
            strcpy(npc->chatLines[2], "I prefer the open road to city life.");
            strcpy(npc->chatLines[3], "You meet all kinds on the road.");
            
            // Quests (8% chance)
            npc->hasQuest = (rand() % 8 == 0);
            if (npc->hasQuest) {
                const char* quests[] = {
                    "I lost a family heirloom on the road.",
                    "Can you deliver a message to my brother?",
                    "I need a guide through the forest.",
                    "Bandits stole my pack! Can you recover it?",
                    "There's a shortcut through the mountains."
                };
                strcpy(npc->questText, quests[rand() % 5]);
            }
            
            strcpy(npc->farewell, "May the road rise to meet you.");
            break;
            
        case NPC_BANDIT:
            npc->isHostile = true;
            npc->canTalk = true; // Can still talk, but will be hostile
            npc->health = 20 + (rand() % 20);
            npc->maxHealth = npc->health;
            npc->gold = 10 + (rand() % 40);
            
            // Greetings
            {
                const char* greetings[] = {
                    "Your gold or your life!",
                    "This is our turf!",
                    "Pay the toll!",
                    "Nice day for a robbery.",
                    "Hand it over!",
                    "Don't try anything funny.",
                    "Easy mark...",
                    "Time to pay up!"
                };
                strcpy(npc->greeting, greetings[rand() % 8]);
            }
            
            // Chat lines (bandits don't chat much)
            npc->chatLineCount = 2;
            strcpy(npc->chatLines[0], "The boss doesn't like talkers.");
            strcpy(npc->chatLines[1], "Just give us your gold and go.");
            
            // No quests for bandits
            npc->hasQuest = false;
            
            strcpy(npc->farewell, "Get out of here!");
            break;
            
        case NPC_WIZARD:
            npc->isHostile = false;
            npc->health = 15 + (rand() % 20);
            npc->maxHealth = npc->health;
            npc->gold = 50 + (rand() % 200);
            
            // Greetings
            {
                const char* greetings[] = {
                    "Ah, a visitor!",
                    "The stars foretold your arrival.",
                    "Careful with the artifacts!",
                    "I've been expecting you.",
                    "Welcome to my tower.",
                    "Knowledge is power!",
                    "Don't touch that!",
                    "Fascinating..."
                };
                strcpy(npc->greeting, greetings[rand() % 8]);
            }
            
            // Chat lines
            npc->chatLineCount = 4;
            strcpy(npc->chatLines[0], "I've been studying the ancient texts.");
            strcpy(npc->chatLines[1], "Magic flows through everything.");
            strcpy(npc->chatLines[2], "The council doesn't approve of my experiments.");
            strcpy(npc->chatLines[3], "I can teach you, for a price.");
            
            // Quests (25% chance - wizards are quest-givers)
            npc->hasQuest = (rand() % 4 == 0);
            if (npc->hasQuest) {
                const char* quests[] = {
                    "I need a rare component from the dungeon.",
                    "Find my lost spellbook in the ruins.",
                    "A magical creature is terrorizing the village.",
                    "I need you to deliver a message to the archmage.",
                    "Help me gather reagents for a powerful spell."
                };
                strcpy(npc->questText, quests[rand() % 5]);
            }
            
            npc->canTrade = (rand() % 2 == 0); // Some wizards sell potions
            if (npc->canTrade) {
                strcpy(npc->tradeText, "I have potions and scrolls for sale.");
            }
            
            strcpy(npc->farewell, "May magic guide your path.");
            break;
    }
    
    // Set base AI properties
    npc->state = NPC_IDLE;
    npc->wanderTimer = 30 + (rand() % 180);
    npc->wanderDirection = rand() % 4;
    npc->givesQuests = npc->hasQuest;
    npc->relationship = 0;
    npc->respawnTimer = 0;
    
    return npc;
}

NPC* GetNPCAt(int worldX, int worldY, int localX, int localY)
{
    for (int i = 0; i < npcCount; i++) {
        NPC* n = npcs[i];
        if (n && n->isInLocalMap && n->state != NPC_DEAD &&
            n->worldX == worldX && n->worldY == worldY && n->localX == localX && n->localY == localY)
            return n;
    }
    return NULL;
}

void UpdateNPCs()
{
    if (!player.isInLocalMap || !npcs) return;
    
    LocalMap* map = worldMap[player.worldPos.y][player.worldPos.x].localMap;
    if (!map) return;
    
    for (int i = 0; i < npcCount; i++) {
        NPC* npc = npcs[i];
        
        // Skip if not in current map or dead
        if (!npc->isInLocalMap || npc->worldX != player.worldPos.x || 
            npc->worldY != player.worldPos.y || npc->state == NPC_DEAD)
            continue;
        
        // Simple wandering AI
        if (npc->state == NPC_IDLE || npc->state == NPC_WANDERING) {
            npc->wanderTimer--;
            
            if (npc->wanderTimer <= 0) {
                if (npc->state == NPC_IDLE) {
                    npc->state = NPC_WANDERING;
                    npc->wanderTimer = 20 + (rand() % 100);
                } else {
                    // Try to move in current direction
                    int newX = npc->localX;
                    int newY = npc->localY;
                    
                    switch (npc->wanderDirection) {
                        case 0: newX++; break;
                        case 1: newX--; break;
                        case 2: newY++; break;
                        case 3: newY--; break;
                    }
                    
                    // Check if move is valid (not a wall and within bounds)
                    if (map && newX > 0 && newX < map->width-1 && 
                        newY > 0 && newY < map->height-1 &&
                        IsTileWalkable(map->tiles[newY][newX]) &&
                        !(newX == player.localPos.x && newY == player.localPos.y) &&
                        !GetNPCAt(npc->worldX, npc->worldY, newX, newY)) 
                    {
                        
                        // Also check if too far from spawn point
                        if (npc->spawnPoint) 
                        {
                            int distFromSpawn = abs(newX - npc->spawnPoint->x) + abs(newY - npc->spawnPoint->y);
                            if (distFromSpawn <= 8) { // Max wander distance
                                npc->localX = newX;
                                npc->localY = newY;
                            }
                        } 
                        else 
                        {
                            npc->localX = newX;
                            npc->localY = newY;
                        }
                    }
                    
                    // Change direction sometimes
                    if (rand() % 4 == 0)
                        npc->wanderDirection = rand() % 4;
                    
                    npc->wanderTimer = 20 + (rand() % 100);
                    
                    // Chance to go idle
                    if (rand() % 8 == 0)
                        npc->state = NPC_IDLE;
                }
            }
        }
    }
}

void DrawNPCs()
{
    if (!player.isInLocalMap || !npcs) return;
    
    for (int i = 0; i < npcCount; i++) {
        NPC* npc = npcs[i];
        
        // Only draw if active and in current map
        if (!npc->isActive || !npc->isInLocalMap || 
            npc->worldX != player.worldPos.x || 
            npc->worldY != player.worldPos.y || 
            npc->state == NPC_DEAD)
            continue;
        
        // black cell behind the glyph so creatures stand out from grass / trees
        DrawGlyphCell((unsigned char)npc->displayChar, npc->localX, npc->localY, npc->color, (Color){ 0, 0, 0, 255 });
        
        // Show name when nearby
        int distX = abs(npc->localX - player.localPos.x);
        int distY = abs(npc->localY - player.localPos.y);
        if (distX < 6 && distY < 6) {
            Vector2 namePos = {
                (float)(npc->localX * TILE_SIZE),
                npc->localY * TILE_SIZE - 18.0f
            };
            
            Color nameColor = npc->isUnique ? GOLD : LIGHTGRAY;
            DrawTextEx(GetFontDefault(), npc->name, namePos, 16, 1, nameColor);
            
            // DEBUG: Print to console to verify names
            // printf("Drawing NPC: '%s' at (%d,%d)\n", npc->name, npc->localX, npc->localY);
        }
    }
}

void InteractWithNPC()
{
    if (!player.isInLocalMap || !npcs) return;
    
    for (int i = 0; i < npcCount; i++) {
        NPC* npc = npcs[i];
        
        if (!npc->isInLocalMap || npc->worldX != player.worldPos.x || 
            npc->worldY != player.worldPos.y || npc->state == NPC_DEAD)
            continue;
        
        int distX = abs(npc->localX - player.localPos.x);
        int distY = abs(npc->localY - player.localPos.y);
        
        if (distX <= 1 && distY <= 1 && npc->canTalk) {
            // Start conversation system
            if (npc->isHostile) {
                // Combat would start here
                printf("[%s]: %s (HOSTILE!)\n", npc->name, npc->greeting);
                // Don't start conversation with hostile NPCs
            } else {
                // Start the conversation system
                printf("Starting conversation with: %s\n", npc->name); // Debug
                StartConversation(npc);
            }
            break;
        }
    }
}

void NPCTakeDamage(NPC* npc, int damage)
{
    npc->health -= damage;
    
    if (npc->health <= 0) {
        npc->state = NPC_DEAD;
        NPCDied(npc);
        
        // Death message based on type
        const char* deathMessages[] = {
            " dies.",
            " falls to the ground.",
            " lets out a final cry.",
            " breathes their last.",
            " collapses."
        };
        printf("%s%s\n", npc->name, deathMessages[rand() % 5]);
    } else {
        // Hit reaction
        const char* hitMessages[] = {
            " takes damage!",
            " grunts in pain.",
            " staggers back.",
            " is hit!",
            " cries out!"
        };
        printf("%s%s\n", npc->name, hitMessages[rand() % 5]);
        
        // Chance to flee if low health
        if (npc->health < npc->maxHealth / 3 && rand() % 3 == 0) {
            npc->state = NPC_FLEEING;
            printf("%s tries to flee!\n", npc->name);
        }
    }
}
