#include "MG1Math/Vect4.h"
#include <math.h>

Vect4::Vect4(const float table[4])
{
    this->x = table[0];
    this->y = table[1];
    this->z = table[2];
    this->w = table[3];
}

Vect4::Vect4(float x, float y, float z, float w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

Vect4::Vect4(Vect3 vec, float w)
{
    this->x = vec.x;
    this->y = vec.y;
    this->z = vec.z;
    this->w = w;
}

Vect4::Vect4(float a)
{
    this->x = a;
    this->y = a;
    this->z = a;
    this->w = a;
}

Vect4& Vect4::operator +=(const Vect4& vec)
{
    this->x += vec.x;
    this->y += vec.y;
    this->z += vec.z;
    this->w += vec.w;
    return *this;
}

Vect4 operator +(Vect4 vec1, const Vect4& vec2)
{
    vec1 += vec2;
    return vec1;
}

Vect4& Vect4::operator -=(const Vect4& vec)
{
    this->x -= vec.x;
    this->y -= vec.y;
    this->z -= vec.z;
    this->w -= vec.w;
    return *this;
}

Vect4 operator -(Vect4 vec1, const Vect4& vec2)
{
    vec1 -= vec2;
    return vec1;
}

Vect4& Vect4::operator *=(float a)
{
    this->x *= a;
    this->y *= a;
    this->z *= a;
    this->w *= a;
    return *this;
}

Vect4 operator *(Vect4 vec, float a)
{
    vec *= a;
    return vec;
}

Vect4 operator*(float a, const Vect4& vec)
{
    return vec * a;
}

Vect4& Vect4::operator *=(const Vect4& vec)
{
    this->x *= vec.x;
    this->y *= vec.y;
    this->z *= vec.z;
    this->w *= vec.w;
    return *this;
}

Vect4 operator *(Vect4 vec1, const Vect4& vec2)
{
    vec1 *= vec2;
    return vec1;
}

Vect4& Vect4::operator /=(float a)
{
    this->x /= a;
    this->y /= a;
    this->z /= a;
    this->w /= a;
    return *this;
}

Vect4 operator /(Vect4 vec, float a)
{
    vec /= a;
    return vec;
}


float Vect4::dot(const Vect4& vec1, const Vect4& vec2)
{
    float sum = 0.0f;
    sum += vec1.x * vec2.x;
    sum += vec1.y * vec2.y;
    sum += vec1.z * vec2.z;
    sum += vec1.w * vec2.w;
    return sum;
}

float Vect4::length() const
{
    return sqrt(this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w);
}


Vect4 Vect4::normalize() const
{
    float l = length();
    if(l == 0.0f)
        return {this->x, this->y, this->z, this->w};

    return {this->x / l, this->y / l, this->z / l, this->w / l};
}

Vect3 Vect4::toVect3() const
{
    return {this->x, this->y, this->z};
}