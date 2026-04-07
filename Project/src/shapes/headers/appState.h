#pragma once
#include "enums.h"

// Trzyma globalny stan aplikacji
struct AppState {
    TransformMode transformMode = LOCAL;
    InputMode inputMode = INPUT_MOUSE;
    InputMode prevInputMode = INPUT_MOUSE;
    DragMode currentMode = BOX;
};