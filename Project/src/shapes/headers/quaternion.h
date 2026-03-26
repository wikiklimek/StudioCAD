#pragma once
#include "MG1Math/Mat4.h"
#include <complex>

struct Quaternion {
    float w = 1.0f, x = 0.0f, y = 0.0f, z = 0.0f; // Domyślnie brak obrotu

    Quaternion() = default;
    Quaternion(float _w, float _x, float _y, float _z) : w(_w), x(_x), y(_y), z(_z) {}

    // Normalizacja zapobiega gromadzeniu się błędów numerycznych
    void normalize() {
        float mag = std::sqrt(w*w + x*x + y*y + z*z);
        if (mag > 0.000001f) {
            w /= mag; x /= mag; y /= mag; z /= mag;
        }
    }

    // Mnożenie kwaternionów (składanie obrotów)
    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
                w * q.w - x * q.x - y * q.y - z * q.z,
                w * q.x + x * q.w + y * q.z - z * q.y,
                w * q.y - x * q.z + y * q.w + z * q.x,
                w * q.z + x * q.y - y * q.x + z * q.w
        );
    }

    // Konwersja delt Eulera z myszki na kwaternion
    static Quaternion fromEuler(float rotX, float rotY, float rotZ) {
        float cx = std::cos(rotX * 0.5f); float sx = std::sin(rotX * 0.5f);
        float cy = std::cos(rotY * 0.5f); float sy = std::sin(rotY * 0.5f);
        float cz = std::cos(rotZ * 0.5f); float sz = std::sin(rotZ * 0.5f);

        Quaternion q;
        q.w = cx * cy * cz + sx * sy * sz;
        q.x = sx * cy * cz - cx * sy * sz;
        q.y = cx * sy * cz + sx * cy * sz;
        q.z = cx * cy * sz - sx * sy * cz;
        return q;
    }

    // Tworzenie kwaternionu z osi i kąta (Axis-Angle)
    static Quaternion fromAxisAngle(float axisX, float axisY, float axisZ, float angle)
    {
        // Zabezpieczenie: normalizacja osi obrotu
        float mag = std::sqrt(axisX*axisX + axisY*axisY + axisZ*axisZ);
        if (mag < 0.000001f) {
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

    Mat4 toMat4() const {
        Mat4 m(1.0f);
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = w * x, wy = w * y, wz = w * z;

        // Kolumna 0
        m.table[0] = 1.0f - 2.0f * (yy + zz);
        m.table[1] = 2.0f * (xy + wz); // Był minus, ma być plus
        m.table[2] = 2.0f * (xz - wy); // Był plus, ma być minus

        // Kolumna 1
        m.table[4] = 2.0f * (xy - wz); // Był plus, ma być minus
        m.table[5] = 1.0f - 2.0f * (xx + zz);
        m.table[6] = 2.0f * (yz + wx); // Był minus, ma być plus

        // Kolumna 2
        m.table[8] = 2.0f * (xz + wy); // Był minus, ma być plus
        m.table[9] = 2.0f * (yz - wx); // Był plus, ma być minus
        m.table[10]= 1.0f - 2.0f * (xx + yy);

        return m;
    }
};