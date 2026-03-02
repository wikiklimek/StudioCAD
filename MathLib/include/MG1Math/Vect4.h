#pragma once
#include "MG1Math/Vect3.h"

class Vect4{
public:
    float x, y, z, w;

    Vect4(const float table[4]);
    Vect4(float x, float y, float z, float w);
    Vect4(Vect3 Vect, float w);
    Vect4(float a);

    Vect4& operator +=(const Vect4& Vect);
    Vect4& operator -=(const Vect4& Vect);
    Vect4& operator *=(float a);
    Vect4& operator /=(float a);
    Vect4& operator *=(const Vect4& Vect);



    friend Vect4 operator *(Vect4 Vect, float a);
    friend Vect4 operator *(float a, const Vect4& Vect);
    friend Vect4 operator /(Vect4 Vect, float a);
    friend Vect4 operator +(Vect4 Vect1, const Vect4& Vect2);
    friend Vect4 operator -(Vect4 Vect1, const Vect4& Vect2);
    friend Vect4 operator *(Vect4 Vect1, const Vect4& Vect2);

    static float dot(const Vect4& Vect1, const Vect4& Vect2);

    float length() const;
    Vect4 normalize() const;

    Vect3 toVect3() const;
};