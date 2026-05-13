#pragma once
#include "quaternion.h"

struct Transformations{
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float scale = 1.0f;

    Quaternion rotation;

    Transformations() = default;
    Transformations(const Vect3& pos) : posX(pos.x), posY(pos.y), posZ(pos.z) {}

    Vect3 getPosition() const { return Vect3(posX, posY, posZ); }
    void setPosition(const Vect3& pos) { posX = pos.x; posY = pos.y; posZ = pos.z; }
};