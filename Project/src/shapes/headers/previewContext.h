#pragma once
#include "quaternion.h"
#include <vector>
#include <memory>


struct PreviewContext{
    bool isTransforming = false;
    bool isLocal = false;
    bool isVirtualSelected = false;

    bool anySelectionChanged = false;
    bool wasBaked = false;


    Vect3 localDeltaPos = Vect3(0.0f, 0.0f, 0.0f);
    float localDeltaScale = 1.0f;
    Quaternion localDeltaRot = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

    // gotowa macierz transformacji
    Mat4 groupMat = Mat4(1.0f);
};