#pragma once
#include "MG1Math/Mat4.h"
#include <complex>

struct Quaternion {
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f; // Domyślnie brak obrotu

    Quaternion() = default;
    Quaternion(float _w, float _x, float _y, float _z) : w(_w), x(_x), y(_y), z(_z) {}

    void normalize();

    Quaternion operator*(const Quaternion& q) const;

    static Quaternion fromAxisAngle(float axisX, float axisY, float axisZ, float angle);


    Mat4 toMat4() const ;
};