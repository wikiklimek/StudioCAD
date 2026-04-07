#pragma once
#include "quaternion.h"
#include <vector>
#include <memory>


struct PreviewContext {
    bool isTransforming = false;
    bool isLocal = false;
    bool isEntireScene = false;

    // Dane do transformacji lokalnej
    Vect3 localDeltaPos = Vect3(0.0f, 0.0f, 0.0f);
    float localDeltaScale = 1.0f;
    Quaternion localDeltaRot = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

    // Gotowa macierz do transformacji grupowej
    Mat4 groupMat = Mat4(1.0f);
};