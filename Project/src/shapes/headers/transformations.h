#pragma once
#include "MG1Math/Vect3.h"
#include "MG1Math/Mat4.h"
#include "quaternion.h"

struct Transformations{
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float scale = 1.0f;

    Quaternion rotation; // Przechowuje aktualny obrót w 4 zmiennych

    Vect3 getPosition() const { return Vect3(posX, posY, posZ); }
    void setPosition(const Vect3& pos) { posX = pos.x; posY = pos.y; posZ = pos.z; }
};