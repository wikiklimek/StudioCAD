#pragma once
#include <cmath>

#include <MG1Math/Vect3.h>
#include <MG1Math/Mat4.h>

#include "matrixesModelViewProjection.h"

Mat4 createModelMatrix(Vect3 position, Vect3 rotation, float scale)
{
    Mat4 S = Mat4::scale(Vect3(scale, scale, scale));

    Mat4 Rx = Mat4::rotateX(rotation.x);
    Mat4 Ry = Mat4::rotateY(rotation.y);
    Mat4 Rz = Mat4::rotateZ(rotation.z);

    Mat4 T = Mat4::translate(position);

    return T * Rz * Ry * Rx * S;
}

Mat4 createProjectionMatrix(float fov, float aspect, float n, float f)
{
    Mat4 P(0.0f);

    float tanHalfFov = std::tan(fov / 2.0f);

    P.table[0] = 1.0f / (aspect * tanHalfFov);

    P.table[5] = 1.0f / tanHalfFov;

    //P.table[10] = (f + n) / (f - n);
    P.table[10] = -(f + n) / (f - n);
    //P.table[11] = 1.0f;
    P.table[11] = -1.0f;

    P.table[14] = -(2.0f * f * n) / (f - n);

    return P;
}


Mat4 oldcreateViewMatrix(Vect3 cameraPos, Vect3 target, Vect3 up)
{
    Vect3 cameraToTarget = (target - cameraPos).normalize();

    Vect3 right = Vect3::cross(cameraToTarget, up).normalize();

    Vect3 upReal = Vect3::cross(right, cameraToTarget).normalize();

    Mat4 view(0.0f);

    // Budowa macierzy rotacji kamery
    view.table[0] = right.x;  view.table[4] = right.y;  view.table[8]  = right.z;
    view.table[1] = upReal.x; view.table[5] = upReal.y; view.table[9]  = upReal.z;
    view.table[2] = cameraToTarget.x;    view.table[6] = cameraToTarget.y;    view.table[10] = cameraToTarget.z;
    view.table[15] = 1.0f;

    // Translacja
    view.table[12] = -Vect3::dot(right, cameraPos);
    view.table[13] = -Vect3::dot(upReal, cameraPos);
    view.table[14] = -Vect3::dot(cameraToTarget, cameraPos);

    return view;
}

Mat4 createViewMatrix(Vect3 cameraPos, Vect3 target, Vect3 up)
{
    // 1. Oś Z (D) - Patrzy od celu do kamery (dodatnie Z widoku)
    Vect3 D = (cameraPos - target).normalize();

    // 2. Oś X (R) - Prawo
    Vect3 R = Vect3::cross(up, D).normalize();

    // 3. Oś Y (U) - Góra
    Vect3 U = Vect3::cross(D, R).normalize();

    Mat4 view(0.0f); // Czyścimy macierz

    // Budowa macierzy - układ Column-Major
    view.table[0] = R.x;  view.table[4] = R.y;  view.table[8]  = R.z;
    view.table[1] = U.x;  view.table[5] = U.y;  view.table[9]  = U.z;
    view.table[2] = D.x;  view.table[6] = D.y;  view.table[10] = D.z;

    // Niezbędne jedynki dla współrzędnych jednorodnych
    view.table[15] = 1.0f;

    // Translacja świata względem kamery
    view.table[12] = -Vect3::dot(R, cameraPos);
    view.table[13] = -Vect3::dot(U, cameraPos);
    view.table[14] = -Vect3::dot(D, cameraPos);

    return view;
}
