#pragma once
#include <iostream>
#include <cmath>

#include <MG1Math/Vect3.h>
#include <MG1Math/Vect4.h>
#include <MG1Math/Mat3.h>
#include <MG1Math/Mat4.h>

#include "matrixesModelViewProjection.h"

Mat4 createModelMatrix(Vect3 position, Vect3 rotation, float scale)
{
    // 1. Skalowanie
    Mat4 S = Mat4::scale(Vect3(scale, scale, scale));

    // 2. Rotacje wokół poszczególnych osi
    Mat4 Rx = Mat4::rotateX(rotation.x);
    Mat4 Ry = Mat4::rotateY(rotation.y);
    Mat4 Rz = Mat4::rotateZ(rotation.z);

    // 3. Translacja (przesunięcie)
    Mat4 T = Mat4::translate(position);

    // Zgodnie z wytycznymi z dokumentu, składanie kolejnych
    // przekształceń to lewostronne mnożenie macierzy (M' = AM)
    // Najpierw obiekt jest skalowany, potem obracany, a na końcu przesuwany.
    return T * Rz * Ry * Rx * S;
}

Mat4 createProjectionMatrix(float fov, float aspect, float n, float f)
{
    Mat4 P(0.0f); // Inicjalizujemy zerami

    float tanHalfFov = std::tan(fov / 2.0f);

    // Implementacja wzoru rzutowania perspektywicznego z zadania.
    // Indeksy zakładają układ column-major: indeks = (kolumna * 4) + wiersz

    // Kolumna 0
    P.table[0] = 1.0f / (aspect * tanHalfFov);

    // Kolumna 1
    P.table[5] = 1.0f / tanHalfFov;

    // Kolumna 2
    P.table[10] = (f + n) / (f - n);
    P.table[11] = 1.0f;

    // Kolumna 3
    P.table[14] = -(2.0f * f * n) / (f - n);

    return P;
}


Mat4 createViewMatrix(Vect3 cameraPos, Vect3 target, Vect3 up)
{
    // 1. Wektor kierunku patrzenia
    Vect3 fwd = (target - cameraPos).normalize();

    // 2. POPRAWKA: Prawidłowy iloczyn wektorowy dla osi X (Prawo)
    Vect3 right = Vect3::cross(fwd, up).normalize();

    // 3. Prawidłowy iloczyn wektorowy dla osi Y (Góra)
    Vect3 upReal = Vect3::cross(right, fwd).normalize();

    Mat4 view(0.0f);

    // Budowa macierzy rotacji kamery
    view.table[0] = right.x;  view.table[4] = right.y;  view.table[8]  = right.z;
    view.table[1] = upReal.x; view.table[5] = upReal.y; view.table[9]  = upReal.z;
    view.table[2] = fwd.x;    view.table[6] = fwd.y;    view.table[10] = fwd.z;
    view.table[15] = 1.0f;

    // Translacja
    view.table[12] = -Vect3::dot(right, cameraPos);
    view.table[13] = -Vect3::dot(upReal, cameraPos);
    view.table[14] = -Vect3::dot(fwd, cameraPos);

    return view;
}
