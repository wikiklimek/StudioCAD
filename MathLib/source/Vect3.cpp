#include "MG1Math/Vect3.h"
#include <math.h>

Vect3::Vect3(const float table[3])
{
    this->x = table[0];
    this->y = table[1];
    this->z = table[2];
}

Vect3::Vect3(float x, float y, float z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}

Vect3::Vect3(float a)
{
    this->x = a;
    this->y = a;
    this->z = a;
}

Vect3& Vect3::operator +=(const Vect3& Vect)
{
    this->x += Vect.x;
    this->y += Vect.y;
    this->z += Vect.z;
    return *this;
}

Vect3 operator +(Vect3 Vect1, const Vect3& Vect2)
{
    Vect1 += Vect2;
    return Vect1;
}

Vect3& Vect3::operator -=(const Vect3& Vect)
{
    this->x -= Vect.x;
    this->y -= Vect.y;
    this->z -= Vect.z;
    return *this;
}

Vect3 operator -(Vect3 Vect1, const Vect3& Vect2)
{
    Vect1 -= Vect2;
    return Vect1;
}

Vect3& Vect3::operator *=(float a)
{
    this->x *= a;
    this->y *= a;
    this->z *= a;
    return *this;
}

Vect3 operator *(Vect3 Vect, float a)
{
    Vect *= a;
    return Vect;
}

Vect3 operator*(float a, const Vect3& Vect)
{
    return Vect * a;
}

Vect3& Vect3::operator *=(const Vect3& Vect)
{
    this->x *= Vect.x;
    this->y *= Vect.y;
    this->z *= Vect.z;
    return *this;
}

Vect3 operator *(Vect3 Vect1, const Vect3& Vect2)
{
    Vect1 *= Vect2;
    return Vect1;
}

Vect3& Vect3::operator /=(float a)
{
    this->x /= a;
    this->y /= a;
    this->z /= a;
    return *this;
}

Vect3 operator /(Vect3 Vect, float a)
{
    Vect /= a;
    return Vect;
}


float Vect3::dot(const Vect3& Vect1, const Vect3& Vect2)
{
    float sum = 0.0f;
    sum += Vect1.x * Vect2.x;
    sum += Vect1.y * Vect2.y;
    sum += Vect1.z * Vect2.z;
    return sum;
}


//float Vect3::cross(Vect3 Vect1, const Vect3& Vect2){}


float Vect3::length() const
{
    return sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
}


Vect3 Vect3::normalize() const
{
    float l = length();
    if(l == 0.0f)
        return {this->x, this->y, this->z};

    return {this->x / l, this->y / l, this->z / l};
}