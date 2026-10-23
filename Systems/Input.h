#ifndef INPUT_H
#define INPUT_H

#include "../project.h"

void HandleInput();

// One movement step per call with key-repeat: fires immediately on a new key press, then again after
// firstDelay seconds, then every repeatRate seconds while held. Returns true when a step should be taken.
bool ReadMoveStep(int* dx, int* dy, float firstDelay, float repeatRate);

#endif
