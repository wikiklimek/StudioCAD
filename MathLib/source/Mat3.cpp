#include "MG1Math/Mat3.h"

Mat3::Mat3(float a)
{
    for(int i = 0; i < Mat3::size_all; i++)
        this->table[i] = 0.0f;

    for(int i = 0; i < Mat3::size1; i++)
        this->table[i * size1 + i] = a;
}

Mat3::Mat3(float a1, float a2, float a3)
{
    for(int i = 0; i < Mat3::size_all; i++)
        this->table[i] = 0.0f;

    this->table[0] = a1;
    this->table[size1 + 1] = a2;
    this->table[2 * size1 + 2] = a3;
}

Mat3::Mat3(const float in[Mat3::size_all])
{
    for(int i = 0; i < Mat3::size_all; i++)
        this->table[i] = in[i];
}


Mat3& Mat3::operator +=(const Mat3& mat)
{
    for(int i = 0; i< Mat3::size_all; i++)
        this->table[i] += mat.table[i];

    return *this;
}

Mat3& Mat3::operator *=(const Mat3& mat)
{
    float out[Mat3::size_all];
    for(int i = 0; i< Mat3::size_all; i++)
        out[i] = 0.0f;


    for(int i = 0; i < Mat3::size1; i++)
        for(int j = 0; j < Mat3::size2; j++)
            for(int k = 0; k < Mat3::size2; k++)
                out[i * size1 + j] += this->table[k * size1 + j] * mat.table[i * size1 + k];

    for(int i = 0; i < Mat3::size_all; i++)
        this->table[i] = out[i];

    return *this;
}

Mat3& Mat3::operator *=(float a)
{
    for(int i = 0; i< Mat3::size_all; i++)
        this->table[i] *= a;

    return *this;
}


Mat3 operator +(Mat3 mat1, const Mat3& mat2)
{
    mat1 += mat2;
    return mat1;
}

Mat3 operator *(Mat3 mat1, const Mat3& mat2)
{
    mat1 *= mat2;
    return mat1;
}

Mat3 operator *(Mat3 mat, float a)
{
    mat *= a;
    return mat;
}

Mat3 operator *(float a, const Mat3& mat)
{
    return mat * a;
}

Vect3 operator *(const Vect3& vec, const Mat3& mat)
{
    float new_table[Mat3::size2] = {0.0f};
    float temp_table[Mat3::size1] = {vec.x, vec.y, vec.z};

    for(int i =0; i< Mat3::size1; i++)
        for(int j =0; j< Mat3::size2; j++)
            new_table[i] += temp_table[j] * mat.table[i * Mat3::size1 + j];

    return {new_table};
}

Vect3 operator *(const Mat3& mat, const Vect3& vec)
{
    float new_table[Mat3::size2] = {0.0f};
    float temp_table[Mat3::size1] = {vec.x, vec.y, vec.z};

    for(int i =0; i< Mat3::size1; i++)
        for(int j =0; j< Mat3::size2; j++)
            new_table[i] += temp_table[j] * mat.table[i + j * Mat3::size2];

    return {new_table};
}


Mat3 Mat3::transpose() const
{
    float new_table[Mat3::size_all];

    for(int i =0; i< Mat3::size1; i++)
        for(int j =0; j< Mat3::size2; j++)
            new_table[i * Mat3::size1 + j] = this->table[j * Mat3::size1 + i];

    return {new_table};
}


//Mat3 Mat3::inverse() const {}
