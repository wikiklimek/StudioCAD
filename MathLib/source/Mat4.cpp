#include "MG1Math/Mat4.h"
#include <math.h>

Mat4::Mat4(float a)
{
    for(int i = 0; i < Mat4::size_all; i++)
        this->table[i] = 0.0f;

    for(int i = 0; i < Mat4::size1; i++)
        this->table[i * size1 + i] = a;
}

Mat4::Mat4(float a1, float a2, float a3, float a4)
{
    for(int i = 0; i < Mat4::size_all; i++)
        this->table[i] = 0.0f;

    this->table[0] = a1;
    this->table[size1 + 1] = a2;
    this->table[2 * size1 + 2] = a3;
    this->table[3 * size1 + 3] = a4;
}

Mat4::Mat4(const float in[Mat4::size_all])
{
    for(int i = 0; i < Mat4::size_all; i++)
        this->table[i] = in[i];
}


Mat4& Mat4::operator +=(const Mat4& mat)
{
    for(int i = 0; i< Mat4::size_all; i++)
        this->table[i] += mat.table[i];

    return *this;
}

Mat4& Mat4::operator *=(const Mat4& mat)
{
    float out[Mat4::size_all];
    for(int i = 0; i< Mat4::size_all; i++)
        out[i] = 0.0f;


    for(int i = 0; i < Mat4::size1; i++)
        for(int j = 0; j < Mat4::size2; j++)
            for(int k = 0; k < Mat4::size2; k++)
                out[i * size1 + j] += this->table[k * size1 + j] * mat.table[i * size1 + k];

    for(int i = 0; i < Mat4::size_all; i++)
        this->table[i] = out[i];

    return *this;
}

Mat4& Mat4::operator *=(float a)
{
    for(int i = 0; i< Mat4::size_all; i++)
        this->table[i] *= a;

    return *this;
}


Mat4 operator +(Mat4 mat1, const Mat4& mat2)
{
    mat1 += mat2;
    return mat1;
}

Mat4 operator *(Mat4 mat1, const Mat4& mat2)
{
    mat1 *= mat2;
    return mat1;
}

Mat4 operator *(Mat4 mat, float a)
{
    mat *= a;
    return mat;
}

Mat4 operator *(float a, const Mat4& mat)
{
    return mat * a;
}

Vect4 operator *(const Vect4& vec, const Mat4& mat)
{
    float new_table[Mat4::size2] = {0.0f};
    float temp_table[Mat4::size1] = {vec.x, vec.y, vec.z, vec.w};

    for(int i =0; i< Mat4::size1; i++)
        for(int j =0; j< Mat4::size2; j++)
            new_table[i] += temp_table[j] * mat.table[i * Mat4::size1 + j];

    return {new_table};
}

Vect4 operator *(const Mat4& mat, const Vect4& vec)
{
    float new_table[Mat4::size2] = {0.0f};
    float temp_table[Mat4::size1] = {vec.x, vec.y, vec.z, vec.w};

    for(int i =0; i< Mat4::size1; i++)
        for(int j =0; j< Mat4::size2; j++)
            new_table[i] += temp_table[j] * mat.table[i + j * Mat4::size2];

    return {new_table};
}


Mat4 Mat4::transpose() const
{
    float new_table[Mat4::size_all];

    for(int i =0; i< Mat4::size1; i++)
        for(int j =0; j< Mat4::size2; j++)
            new_table[i * size1 + j] = this->table[j * size1 + i];

    return {new_table};
}

//Mat4 Mat4::inverse() const {}

Mat4 Mat4::translate(const Vect3& v)
{
    Mat4 mat(1.0f);
    mat.table[Mat4::size_all - Mat4::size1] = v.x;
    mat.table[Mat4::size_all - Mat4::size1 + 1] = v.y;
    mat.table[Mat4::size_all - Mat4::size1 + 2] = v.z;

    return mat;
}

Mat4 Mat4::scale(const Vect3& v)
{
    return {v.x, v.y, v.z, 1.0};
}

Mat4 Mat4::rotateX(float angle)
{
    Mat4 mat(1.0f);

    float c = cos(angle);
    float s = sin(angle);

    mat.table[5] = c;
    mat.table[6] = s;
    mat.table[9] = -s;
    mat.table[10] = c;

    return mat;
}

Mat4 Mat4::rotateY(float angle)
{
    Mat4 mat(1.0f);

    float c = cos(angle);
    float s = sin(angle);

    mat.table[0] = c;
    mat.table[2] = -s;
    mat.table[8] = s;
    mat.table[10] = c;

    return mat;
}

Mat4 Mat4::rotateZ(float angle)
{
    Mat4 mat(1.0f);

    float c = cos(angle);
    float s = sin(angle);

    mat.table[0] = c;
    mat.table[1] = s;
    mat.table[4] = -s;
    mat.table[5] = c;

    return mat;
}

Mat4 Mat4::translate_inverse(const Vect3& v)
{
    Mat4 mat(1.0f);
    mat.table[Mat4::size_all - Mat4::size1] = -v.x;
    mat.table[Mat4::size_all - Mat4::size1 + 1] = -v.y;
    mat.table[Mat4::size_all - Mat4::size1 + 2] = -v.z;

    return mat;
}

Mat4 Mat4::scale_inverse(const Vect3& v)
{
    return {1/v.x, 1/v.y, 1/v.z, 1.0};
}

Mat4 Mat4::rotateX_inverse(float angle)
{
    Mat4 mat(1.0f);

    float c = cos(angle);
    float s = sin(angle);

    mat.table[5] = c;
    mat.table[6] = -s;
    mat.table[9] = s;
    mat.table[10] = c;

    return mat;
}

Mat4 Mat4::rotateY_inverse(float angle)
{
    Mat4 mat(1.0f);

    float c = cos(angle);
    float s = sin(angle);

    mat.table[0] = c;
    mat.table[2] = s;
    mat.table[8] = -s;
    mat.table[10] = c;

    return mat;
}

Mat4 Mat4::rotateZ_inverse(float angle)
{
    Mat4 mat(1.0f);

    float c = cos(angle);
    float s = sin(angle);

    mat.table[0] = c;
    mat.table[1] = -s;
    mat.table[4] = s;
    mat.table[5] = c;

    return mat;
}
