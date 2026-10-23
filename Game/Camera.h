#ifndef CAMERA_H
#define CAMERA_H

#include "../project.h"

void InitCamera();                    // Original (resets zoom)
void InitCamera(bool resetZoom);       // Version with control
void SafeInitCamera(bool resetZoom);   // NEW: Safe version with validation
void UpdateCamera();
void ResetCamera();
void ToggleFullscreen(int windowWidth, int windowHeight);
float FitWorldZoom();     // zoom at which the whole world map fits in the window
Vector2 GetWorldToScreen(Vector2 worldPos);

#endif
