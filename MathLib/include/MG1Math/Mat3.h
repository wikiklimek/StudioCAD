#pragma once
#include "MG1Math/Vect3.h"

class Mat3{
public:
    static const int size1 = 3;
    static const int size2 = 3;
    static const int size_all = size1 * size2;
    float table[size_all];

    Mat3(float a);
    Mat3(float a1, float a2, float a3);
    Mat3(const float table[size_all]);

    Mat3& operator +=(const Mat3& mat);
    Mat3& operator *=(const Mat3& mat);
    Mat3& operator *=(float a);

    friend Mat3 operator +(Mat3 mat1, const Mat3& mat2);
    friend Mat3 operator *(Mat3 mat1, const Mat3& mat2);
    friend Mat3 operator *(Mat3 mat, float a);
    friend Mat3 operator *(float a, const Mat3& mat);

    friend Vect3 operator *(const Vect3& vec, const Mat3& mat);
    friend Vect3 operator *(const Mat3& mat, const Vect3& vec);


    Mat3 transpose() const;
    //Mat3 inverse() const;
};