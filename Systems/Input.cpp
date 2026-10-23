#include "Input.h"
#include "../Entities/Player.h"
#include "../Game/Camera.h"

bool ReadMoveStep(int* dx, int* dy, float firstDelay, float repeatRate)
{
    static int lastDx = 0, lastDy = 0;
    static double nextStep = 0.0;

    int ix = ((IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) ? 1 : 0) - ((IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) ? 1 : 0);
    int iy = ((IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) ? 1 : 0) - ((IsKeyDown(KEY_UP)   || IsKeyDown(KEY_W)) ? 1 : 0);

    if (ix == 0 && iy == 0) { lastDx = lastDy = 0; return false; }

    bool vertPressed = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    bool horzPressed = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    if (ix != 0 && iy != 0) {          // 4-directional movement: the most recently pressed axis wins
        if (vertPressed && !horzPressed) ix = 0;
        else if (horzPressed && !vertPressed) iy = 0;
        else if (lastDx != 0) iy = 0;
        else ix = 0;
    }

    double now = GetTime();
    if (vertPressed || horzPressed || ix != lastDx || iy != lastDy) {
        lastDx = ix; lastDy = iy;
        nextStep = now + firstDelay;
        *dx = ix; *dy = iy;
        return true;
    }
    if (now >= nextStep) {
        nextStep = now + repeatRate;
        *dx = ix; *dy = iy;
        return true;
    }
    return false;
}

void HandleInput()
{
    UpdatePlayer();

    if (IsKeyPressed(KEY_R))
        ResetCamera();
}
