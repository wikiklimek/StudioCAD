#pragma once
#include <cmath>

#include <MG1Math/Vect3.h>
#include <MG1Math/Mat4.h>

#include "matrixesModelViewProjection.h"
#include "quaternion.h"

Mat4 createModelMatrix(Vect3 position, Quaternion rotation, float scale)
{
    Mat4 S = Mat4::scale(Vect3(scale, scale, scale));
    Mat4 T = Mat4::translate(position);
    Mat4 R = rotation.toMat4(); // Tu z kwaternionu powstaje macierz

    return T * R * S;
}

Mat4 createProjectionMatrix(float fov, float aspect, float n, float f)
{
    Mat4 P(0.0f);

    float tanHalfFov = std::tan(fov / 2.0f);

    P.table[0] = 1.0f / (aspect * tanHalfFov);

    P.table[5] = 1.0f / tanHalfFov;

    //P.table[10] = (f + n) / (f - n);
    //P.table[11] = 1.0f;
    P.table[10] = -(f + n) / (f - n);
    P.table[11] = -1.0f;

    P.table[14] = -(2.0f * f * n) / (f - n);

    return P;
}

Mat4 createViewMatrix(Vect3 cameraPos, Vect3 target, Vect3 up)
{
    //Vect3 D = (target - cameraPos).normalize();
    //Vect3 R = Vect3::cross(D, up).normalize();
    //Vect3 U = Vect3::cross(R, D).normalize();

    Vect3 D = (cameraPos - target).normalize();
    Vect3 R = Vect3::cross(up, D).normalize();
    Vect3 U = Vect3::cross(D, R).normalize();

    Mat4 view(0.0f);


    view.table[0] = R.x;  view.table[4] = R.y;  view.table[8]  = R.z;
    view.table[1] = U.x;  view.table[5] = U.y;  view.table[9]  = U.z;
    view.table[2] = D.x;  view.table[6] = D.y;  view.table[10] = D.z;


    view.table[15] = 1.0f;


    view.table[12] = -Vect3::dot(R, cameraPos);
    view.table[13] = -Vect3::dot(U, cameraPos);
    view.table[14] = -Vect3::dot(D, cameraPos);

    return view;
}
