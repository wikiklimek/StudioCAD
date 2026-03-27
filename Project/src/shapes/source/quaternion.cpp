#pragma once
#include "quaternion.h"


void Quaternion::normalize()
{
    float mag = std::sqrt(w*w + x*x + y*y + z*z);
    if (mag > 0.000001f)
    {
        w /= mag;
        x /= mag;
        y /= mag;
        z /= mag;
    }
}


Quaternion Quaternion::operator*(const Quaternion& q) const {
    return Quaternion(
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
    );
}



Quaternion Quaternion::fromAxisAngle(float axisX, float axisY, float axisZ, float angle)
{

    float mag = std::sqrt(axisX*axisX + axisY*axisY + axisZ*axisZ);
    if (mag < 0.000001f)
    {
        return Quaternion(1.0f, 0.0f, 0.0f, 0.0f); // Brak obrotu
    }

    axisX /= mag;
    axisY /= mag;
    axisZ /= mag;

    float halfAngle = angle * 0.5f;
    float s = std::sin(halfAngle);

    return Quaternion(
            std::cos(halfAngle),
            axisX * s,
            axisY * s,
            axisZ * s
    );
}

Mat4 Quaternion::toMat4() const
{
    Mat4 m(1.0f);
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;


    m.table[0] = 1.0f - 2.0f * (yy + zz);
    m.table[1] = 2.0f * (xy + wz);
    m.table[2] = 2.0f * (xz - wy);


    m.table[4] = 2.0f * (xy - wz);
    m.table[5] = 1.0f - 2.0f * (xx + zz);
    m.table[6] = 2.0f * (yz + wx);


    m.table[8] = 2.0f * (xz + wy);
    m.table[9] = 2.0f * (yz - wx);
    m.table[10]= 1.0f - 2.0f * (xx + yy);

    return m;
}