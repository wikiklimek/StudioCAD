#pragma once
#include "torusGrid.h"
#include <cmath>

#define _USE_MATH_DEFINES
float const PI = (float)M_PI;

void generateTorus(float R, float r, int density_R, int density_r, std::vector<float>& vertices, std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    float step_R = 2.0f * (float)M_PI / (float)density_R;
    float step_r = 2.0f * (float)M_PI / (float)density_r;

    // GEOMETRIA
    for (int i = 0; i <= density_R; ++i)
    {
        float u = i * step_R;

        for (int j = 0; j <= density_r; ++j)
        {
            float v = j * step_r;


            float x = (R + r * std::cos(v)) * std::cos(u);
            float y = (R + r * std::cos(v)) * std::sin(u);
            float z = r * std::sin(v);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // TOPOLOGIA
    for (int i = 0; i < density_R; ++i)
    {
        for (int j = 0; j < density_r; ++j)
        {
            int rowLength = density_r + 1;

            unsigned int p0 = i * rowLength + j;
            unsigned int p_right = (i + 1) * rowLength + j;
            unsigned int p_up = i * rowLength + (j + 1);

            // pozioma
            indices.push_back(p0);
            indices.push_back(p_right);

            // pionowa
            indices.push_back(p0);
            indices.push_back(p_up);


            //przekatna
            //indices.push_back(p_up);
            //indices.push_back(p_right);
        }
    }
}
