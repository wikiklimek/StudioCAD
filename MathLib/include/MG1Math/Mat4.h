#pragma once
#include "MG1Math/Vect4.h"

class Mat4{
public:
    static const int size1 = 4;
    static const int size2 = 4;
    static const int size_all = size1 * size2;
    float table[size_all];

    Mat4(float a);
    Mat4(float a1, float a2, float a3, float a4);
    Mat4(const float table[size_all]);

    Mat4& operator +=(const Mat4& mat);
    Mat4& operator *=(const Mat4& mat);
    Mat4& operator *=(float a);

    friend Mat4 operator +(Mat4 mat1, const Mat4& mat2);
    friend Mat4 operator *(Mat4 mat1, const Mat4& mat2);
    friend Mat4 operator *(Mat4 mat, float a);
    friend Mat4 operator *(float a, const Mat4& mat);


    friend Vect4 operator *(const Vect4& vec, const Mat4& mat);
    friend Vect4 operator *(const Mat4& mat, const Vect4& vec);


    Mat4 transpose() const;
    //Mat4 inverse() const;

    static Mat4 translate(const Vect3& v);
    static Mat4 scale(const Vect3& v);
    static Mat4 rotateX(float angle);
    static Mat4 rotateY(float angle);
    static Mat4 rotateZ(float angle);

    static Mat4 translate_inverse(const Vect3& v);
    static Mat4 scale_inverse(const Vect3& v);
    static Mat4 rotateX_inverse(float angle);
    static Mat4 rotateY_inverse(float angle);
    static Mat4 rotateZ_inverse(float angle);

};
