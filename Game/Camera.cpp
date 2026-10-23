#include "Camera.h"

static float ClampFloat(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Safe version that NEVER uses localPos when not in local map
void SafeInitCamera(bool resetZoom)
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // ALWAYS check isInLocalMap before using localPos
    if (player.isInLocalMap) {
        // Validate local position before using it
        if (player.localPos.x < 0 || player.localPos.x >= LOCAL_MAP_WIDTH ||
            player.localPos.y < 0 || player.localPos.y >= LOCAL_MAP_HEIGHT) {
            printf("CRITICAL: Invalid local pos (%d,%d) in SafeInitCamera - resetting\n",
                   player.localPos.x, player.localPos.y);
            player.localPos.x = LOCAL_MAP_WIDTH / 2;
            player.localPos.y = LOCAL_MAP_HEIGHT / 2;
        }
        
        gameCamera.camera.target = (Vector2){ 
            player.localPos.x * TILE_SIZE + TILE_SIZE / 2.0f,
            player.localPos.y * TILE_SIZE + TILE_SIZE / 2.0f
        };
        
        if (resetZoom) {
            gameCamera.zoom = 1.0f;
        }
    } else {
        // NOT in local map - ONLY use worldPos
        // Validate world position
        if (player.worldPos.x < 0 || player.worldPos.x >= currentMapWidth ||
            player.worldPos.y < 0 || player.worldPos.y >= currentMapHeight) {
            printf("CRITICAL: Invalid world pos (%d,%d) in SafeInitCamera - resetting to map centre\n",
                   player.worldPos.x, player.worldPos.y);
            player.worldPos.x = currentMapWidth / 2;
            player.worldPos.y = currentMapHeight / 2;
        }
        
        gameCamera.camera.target = (Vector2){ 
            player.worldPos.x * TILE_SIZE + TILE_SIZE / 2.0f,
            player.worldPos.y * TILE_SIZE + TILE_SIZE / 2.0f
        };
        
        if (resetZoom) {
            // tiles are 24px now; pick a zoom so a small world fits the window and big worlds stay readable
            if (currentMapWidth <= 32 && currentMapHeight <= 32) {
                gameCamera.zoom = 2.0f;
            } else if (currentMapWidth <= 64 && currentMapHeight <= 64) {
                gameCamera.zoom = 1.6f;
            } else if (currentMapWidth <= 128 && currentMapHeight <= 128) {
                gameCamera.zoom = 1.1f;
            } else {
                gameCamera.zoom = 0.9f;
            }
        }
    }
    
    gameCamera.camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    gameCamera.camera.rotation = 0.0f;
    gameCamera.camera.zoom = gameCamera.zoom;
}

// Original for backward compatibility - but now safe
void InitCamera(bool resetZoom)
{
    SafeInitCamera(resetZoom);
}

void InitCamera()
{
    SafeInitCamera(true);
}

void UpdateCamera()
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    gameCamera.camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    
    Vector2 targetPos;
    
    // CRITICAL: Always check isInLocalMap
    if (player.isInLocalMap) {
        // Validate local position
        if (player.localPos.x < 0 || player.localPos.x >= LOCAL_MAP_WIDTH ||
            player.localPos.y < 0 || player.localPos.y >= LOCAL_MAP_HEIGHT) {
            printf("CRITICAL: Invalid local pos (%d,%d) in UpdateCamera - resetting\n",
                   player.localPos.x, player.localPos.y);
            player.localPos.x = LOCAL_MAP_WIDTH / 2;
            player.localPos.y = LOCAL_MAP_HEIGHT / 2;
        }
        
        targetPos.x = player.localPos.x * TILE_SIZE + TILE_SIZE / 2.0f;
        targetPos.y = player.localPos.y * TILE_SIZE + TILE_SIZE / 2.0f;
    } else {
        // Validate world position
        if (player.worldPos.x < 0 || player.worldPos.x >= currentMapWidth ||
            player.worldPos.y < 0 || player.worldPos.y >= currentMapHeight) {
            printf("CRITICAL: Invalid world pos (%d,%d) in UpdateCamera - resetting to map centre\n",
                   player.worldPos.x, player.worldPos.y);
            player.worldPos.x = currentMapWidth / 2;
            player.worldPos.y = currentMapHeight / 2;
        }
        
        targetPos.x = player.worldPos.x * TILE_SIZE + TILE_SIZE / 2.0f;
        targetPos.y = player.worldPos.y * TILE_SIZE + TILE_SIZE / 2.0f;
    }
    
    // Smooth follow
    float lerpSpeed = 0.15f;
    gameCamera.camera.target.x += (targetPos.x - gameCamera.camera.target.x) * lerpSpeed;
    gameCamera.camera.target.y += (targetPos.y - gameCamera.camera.target.y) * lerpSpeed;
    
    // Mouse wheel zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        float zoomChange = wheel * 0.1f * gameCamera.zoom;
        float newZoom = gameCamera.zoom + zoomChange;
        
        float minZoom, maxZoom;
        if (player.isInLocalMap) {
            minZoom = 0.5f;
            maxZoom = 3.0f;
        } else {
            minZoom = 0.07f;      // a 512x512 world needs ~0.1 to fit a 1280px window
            maxZoom = 4.0f;
        }
        
        newZoom = ClampFloat(newZoom, minZoom, maxZoom);
        
        if (newZoom != gameCamera.zoom) {
            gameCamera.zoom = newZoom;
            gameCamera.camera.zoom = gameCamera.zoom;
        }
    }
}

void ResetCamera()
{
    SafeInitCamera(true);
}

void ToggleFullscreen(int windowWidth, int windowHeight)
{
    if (!IsWindowFullscreen()) {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    } else {
        ToggleFullscreen();
        SetWindowSize(windowWidth, windowHeight);
    }
    
    gameCamera.camera.offset.x = GetScreenWidth() / 2.0f;
    gameCamera.camera.offset.y = GetScreenHeight() / 2.0f;
}

float FitWorldZoom()
{
    float zx = (float)(GetScreenWidth() - 40) / (float)(currentMapWidth * TILE_SIZE);
    float zy = (float)(GetScreenHeight() - 140) / (float)(currentMapHeight * TILE_SIZE);
    float z = zx < zy ? zx : zy;
    return ClampFloat(z, 0.07f, 2.4f);
}
