#pragma once
class Vect3{
public:
    float x, y, z;

    Vect3(float x, float y, float z);
    Vect3(float a);
    Vect3(const float table[3]);


    Vect3& operator +=(const Vect3& Vect);
    Vect3& operator -=(const Vect3& Vect);
    Vect3& operator *=(float a);
    Vect3& operator /=(float a);
    Vect3& operator *=(const Vect3& Vect);



    friend Vect3 operator *(Vect3 Vect, float a);
    friend Vect3 operator *(float a, const Vect3& Vect);
    friend Vect3 operator /(Vect3 Vect, float a);
    friend Vect3 operator +(Vect3 Vect1, const Vect3& Vect2);
    friend Vect3 operator -(Vect3 Vect1, const Vect3& Vect2);
    friend Vect3 operator *(Vect3 Vect1, const Vect3& Vect2);


    static float dot(const Vect3& Vect1, const Vect3& Vect2);
    static Vect3 cross(const Vect3& a, const Vect3& b);

    float length() const;
    Vect3 normalize() const;
};