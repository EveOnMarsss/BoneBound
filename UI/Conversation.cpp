#include "Conversation.h"
#include "../Entities/Player.h"

extern NPC* talkingNPC;
extern bool isInConversation;
extern int conversationChoice;

// Color comparison helper
bool IsColorEqual(Color c1, Color c2) {
    return (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b && c1.a == c2.a);
}

bool IsColorGray(Color c) {
    return (c.r == GRAY.r && c.g == GRAY.g && c.b == GRAY.b);
}

// Draws text wrapped to maxWidth; returns the height used. (Quest text is up to 200 chars and used to overflow the box.)
static int DrawWrapped(const char* text, int x, int y, int maxWidth, int fontSize, Color color)
{
    char line[256] = "";
    int lineCount = 0;
    const int lineHeight = fontSize + 4;
    const char* p = text;
    char word[128];
    while (*p) {
        int wl = 0;
        while (*p == ' ') p++;
        while (*p && *p != ' ' && wl < 127) word[wl++] = *p++;
        word[wl] = '\0';
        if (wl == 0) break;
        char test[256];
        snprintf(test, sizeof(test), "%s%s%s", line, line[0] ? " " : "", word);
        if (line[0] && MeasureTextEx(GetFontDefault(), test, (float)fontSize, 1).x > maxWidth) {
            DrawTextEx(GetFontDefault(), line, (Vector2){ (float)x, (float)(y + lineCount * lineHeight) }, (float)fontSize, 1, color);
            lineCount++;
            snprintf(line, sizeof(line), "%s", word);
        } else {
            snprintf(line, sizeof(line), "%s", test);
        }
    }
    if (line[0]) { DrawTextEx(GetFontDefault(), line, (Vector2){ (float)x, (float)(y + lineCount * lineHeight) }, (float)fontSize, 1, color); lineCount++; }
    return lineCount * lineHeight;
}

void StartConversation(NPC* npc)
{
    if (!npc || !npc->canTalk) return;
    
    talkingNPC = npc;
    isInConversation = true;
    conversationChoice = 0;
    npc->isTalking = true;
    npc->dialogState = DIALOG_GREETING;
}

void EndConversation()
{
    if (talkingNPC) {
        talkingNPC->isTalking = false;
        talkingNPC = NULL;
    }
    isInConversation = false;
    conversationChoice = 0;
}

void UpdateConversation()
{
    if (!isInConversation || !talkingNPC) return;
    
    // Navigation with arrow keys - only move between available options
    int availableOptions = 0;
    int optionMap[4] = {-1, -1, -1, -1}; // Maps menu position to actual option index
    
    if (talkingNPC->dialogState == DIALOG_CHAT) {
        // Determine which options are available
        if (talkingNPC->canTalk) optionMap[availableOptions++] = 0; // Chat always available
        if (talkingNPC->hasQuest) optionMap[availableOptions++] = 1; // Quest
        if (talkingNPC->canTrade) optionMap[availableOptions++] = 2; // Trade
        optionMap[availableOptions++] = 3; // Goodbye always available
        
        // Only allow navigation if there are options
        if (availableOptions > 0) {
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                conversationChoice = (conversationChoice + 1) % availableOptions;
            }
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                conversationChoice = (conversationChoice - 1 + availableOptions) % availableOptions;
            }
        }
    } else {
        // For other dialog states, simple navigation
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            conversationChoice = (conversationChoice + 1) % 2; // Always 2 options for quest/trade
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            conversationChoice = (conversationChoice - 1 + 2) % 2;
        }
    }
    
    // Select option with ENTER or SPACE
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (talkingNPC->dialogState) {
            case DIALOG_GREETING:
                talkingNPC->dialogState = DIALOG_CHAT;
                conversationChoice = 0;
                break;
                
            case DIALOG_CHAT:
                {
                    // Rebuild option map
                    availableOptions = 0;
                    int localOptionMap[4] = {-1, -1, -1, -1};
                    
                    if (talkingNPC->canTalk) localOptionMap[availableOptions++] = 0;
                    if (talkingNPC->hasQuest) localOptionMap[availableOptions++] = 1;
                    if (talkingNPC->canTrade) localOptionMap[availableOptions++] = 2;
                    localOptionMap[availableOptions++] = 3;
                    
                    if (availableOptions > 0 && conversationChoice < availableOptions) {
                        int selected = localOptionMap[conversationChoice];
                        
                        switch (selected) {
                            case 0: // Chat
                                // Stay in chat state but show random dialogue
                                break;
                            case 1: // Quest
                                talkingNPC->dialogState = DIALOG_QUEST;
                                break;
                            case 2: // Trade
                                talkingNPC->dialogState = DIALOG_TRADE;
                                break;
                            case 3: // Goodbye
                                EndConversation();
                                return;
                        }
                    }
                    conversationChoice = 0;
                }
                break;
                
            case DIALOG_QUEST:
                if (conversationChoice == 0) {
                    // Accept quest
                    talkingNPC->hasQuest = false;
                    talkingNPC->dialogState = DIALOG_CHAT;
                } else {
                    // Decline
                    talkingNPC->dialogState = DIALOG_CHAT;
                }
                conversationChoice = 0;
                break;
                
            case DIALOG_TRADE:
                if (conversationChoice == 0) {
                    // Open trade menu
                    // Add trade logic here
                    talkingNPC->dialogState = DIALOG_CHAT;
                } else {
                    talkingNPC->dialogState = DIALOG_CHAT;
                }
                conversationChoice = 0;
                break;
                
            default:
                break;
        }
    }
    
    // Exit with ESCAPE
    if (IsKeyPressed(KEY_ESCAPE)) {
        EndConversation();
    }
}

void DrawConversation()
{
    if (!isInConversation || !talkingNPC) return;
    
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Dim background
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 0, 0, 0, 200 });
    
    // Conversation box dimensions
    int boxWidth = 600;
    int boxHeight = 300;
    int boxX = (screenWidth - boxWidth) / 2;
    int boxY = (screenHeight - boxHeight) / 2;
    
    // Draw conversation box
    DrawRectangle(boxX, boxY, boxWidth, boxHeight, BLACK);
    DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, WHITE);
    
    // NPC Name in top left - JUST THE NAME, no "the Cat/Dog"
    DrawTextEx(GetFontDefault(), talkingNPC->name,
               (Vector2){ (float)(boxX + 20), (float)(boxY + 20) }, 24, 1, talkingNPC->color);
    
    // Draw portrait placeholder
    DrawRectangle(boxX + 20, boxY + 60, 60, 60, DARKGRAY);
    DrawTextEx(GetFontDefault(), TextFormat("%c", talkingNPC->displayChar),
               (Vector2){ (float)(boxX + 45), (float)(boxY + 80) }, 40, 1, talkingNPC->color);
    
    // Dialog text area
    int textX = boxX + 100;
    int textY = boxY + 70;
    int textWidth = boxWidth - 120;
    
    switch (talkingNPC->dialogState) {
        case DIALOG_GREETING:
            {
                int h = DrawWrapped(talkingNPC->greeting, textX, textY, textWidth, 20, WHITE);
                DrawTextEx(GetFontDefault(), "Press ENTER to continue...",
                          (Vector2){ (float)textX, (float)(textY + h + 20) }, 16, 1, LIGHTGRAY);
            }
            break;
            
        case DIALOG_CHAT:
            {
                // Show a random chat line - but don't change it every frame
                static int lastLineIndex = -1;
                static float lineChangeTimer = 0;
                int h = 24;
                
                if (talkingNPC->chatLineCount > 0) {
                    // Only change line every few seconds
                    lineChangeTimer += GetFrameTime();
                    if (lineChangeTimer > 3.0f || lastLineIndex == -1) { // Change every 3 seconds
                        lastLineIndex = rand() % talkingNPC->chatLineCount;
                        lineChangeTimer = 0;
                    }
                    
                    h = DrawWrapped(talkingNPC->chatLines[lastLineIndex], textX, textY, textWidth, 20, WHITE);
                } else {
                    h = DrawWrapped("Nice weather we're having!", textX, textY, textWidth, 20, WHITE);
                }
                
                // Options - build menu based on available options
                const char* optionTexts[4] = { "Chat", "Quest", "Trade", "Leave" };
                bool optionAvailable[4] = { true, talkingNPC->hasQuest, talkingNPC->canTrade, true };
                
                // Map available options to display positions
                int displayOrder[4];
                int availableCount = 0;
                
                if (optionAvailable[0]) displayOrder[availableCount++] = 0; // Chat
                if (optionAvailable[1]) displayOrder[availableCount++] = 1; // Quest
                if (optionAvailable[2]) displayOrder[availableCount++] = 2; // Trade
                if (optionAvailable[3]) displayOrder[availableCount++] = 3; // Leave
                
                // Display options
                for (int i = 0; i < availableCount; i++) {
                    int optionIndex = displayOrder[i];
                    bool isSelected = (i == conversationChoice);
                    
                    Color color = isSelected ? YELLOW : (optionAvailable[optionIndex] ? WHITE : GRAY);
                    int y = textY + h + 18 + i * 25;
                    
                    if (isSelected) {
                        DrawTextEx(GetFontDefault(), ">", 
                                  (Vector2){ (float)(textX - 20), (float)y }, 20, 1, YELLOW);
                    }
                    
                    DrawTextEx(GetFontDefault(), optionTexts[optionIndex],
                              (Vector2){ (float)textX, (float)y }, 20, 1, color);
                }
            }
            break;
            
        case DIALOG_QUEST:
            if (talkingNPC->hasQuest) {
                int qh = DrawWrapped(talkingNPC->questText, textX, textY, textWidth, 20, GOLD);
                
                // Accept/Decline options
                const char* options[2] = { "Accept", "Decline" };
                for (int i = 0; i < 2; i++) {
                    Color color = (i == conversationChoice) ? YELLOW : WHITE;
                    int y = textY + qh + 18 + i * 25;
                    
                    if (i == conversationChoice) {
                        DrawTextEx(GetFontDefault(), ">",
                                  (Vector2){ (float)(textX - 20), (float)y }, 20, 1, YELLOW);
                    }
                    
                    DrawTextEx(GetFontDefault(), options[i],
                              (Vector2){ (float)textX, (float)y }, 20, 1, color);
                }
            } else {
                // If quest is no longer available, go back to chat
                talkingNPC->dialogState = DIALOG_CHAT;
                conversationChoice = 0;
            }
            break;
            
        case DIALOG_TRADE:
            {
                int th = DrawWrapped(talkingNPC->tradeText, textX, textY, textWidth, 20, GREEN);
                
                // Trade options
                const char* tradeOptions[2] = { "Browse Wares", "Back" };
                for (int i = 0; i < 2; i++) {
                    Color color = (i == conversationChoice) ? YELLOW : WHITE;
                    int y = textY + th + 18 + i * 25;
                    
                    if (i == conversationChoice) {
                        DrawTextEx(GetFontDefault(), ">",
                                  (Vector2){ (float)(textX - 20), (float)y }, 20, 1, YELLOW);
                    }
                    
                    DrawTextEx(GetFontDefault(), tradeOptions[i],
                              (Vector2){ (float)textX, (float)y }, 20, 1, color);
                }
            }
            break;
            
        default:
            break;
    }
    
    // Instructions at bottom
    const char* inst = "UP/DOWN to navigate, ENTER to select, ESC to exit";
    Vector2 instSize = MeasureTextEx(GetFontDefault(), inst, 14, 1);
    DrawTextEx(GetFontDefault(), inst,
              (Vector2){ (float)(boxX + (int)((boxWidth - instSize.x) / 2)), (float)(boxY + boxHeight - 25) },
              14, 1, LIGHTGRAY);
}