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
    Mat4 R = rotation.toMat4();

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

Mat4 createFrustum(float l, float r, float b, float t, float n, float f)
{
    Mat4 M(0.0f);
    M.table[0] = (2.0f * n) / (r - l);
    M.table[5] = (2.0f * n) / (t - b);
    M.table[8] = (r + l) / (r - l);
    M.table[9] = (t + b) / (t - b);
    M.table[10] = -(f + n) / (f - n);
    M.table[11] = -1.0f;
    M.table[14] = -(2.0f * f * n) / (f - n);
    return M;
}

// Obliczanie asymetrycznych krawędzi (Off-axis projection) dla konkretnego oka
void getStereoMatrices(float fov, float aspect, float n, float f, float eyeSeparation, float zpd, bool isLeftEye, Mat4& outProj, Mat4& outViewShift)
{
    float top = n * std::tan(fov / 2.0f);
    float bottom = -top;
    float a = aspect * top; // To byłoby 'right' w kamerze symetrycznej

    // O ile musimy przesunąć okno rzutowania na bliskiej płaszczyźnie (n),
    // wynikające z podobieństwa trójkątów do płaszczyzny ZPD.
    float shift = (eyeSeparation / 2.0f) * (n / zpd);
    float left, right;

    if (isLeftEye)
    {
        // Lewe oko jest przesunięte w lewo, więc okno monitora 'przesuwa się' w prawo
        left = -a + shift;
        right = a + shift;
        // W przestrzeni widoku (View) przesuwamy całą scenę w prawo (oko w lewo)
        outViewShift = Mat4::translate(Vect3(eyeSeparation / 2.0f, 0.0f, 0.0f));
    } else
    {
        // Prawe oko
        left = -a - shift;
        right = a - shift;
        outViewShift = Mat4::translate(Vect3(-eyeSeparation / 2.0f, 0.0f, 0.0f));
    }

    outProj = createFrustum(left, right, bottom, top, n, f);
}
