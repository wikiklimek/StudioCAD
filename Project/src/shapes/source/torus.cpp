#pragma once
#include "torus.h"
#include <cmath>

#define _USE_MATH_DEFINES
float const PI = (float)M_PI;

void generateTorus(float R, float r, int density_R, int density_r, std::vector<float>& vertices, std::vector<unsigned int>& indices)
{
    // Czyszczenie
    vertices.clear();
    indices.clear();

    // Krok kątowy dla obu pętli (od 0 do 2*PI)
    float step_R = 2.0f * (float)M_PI / (float)density_R;
    float step_r = 2.0f * (float)M_PI / (float)density_r;

    // --- GEOMETRIA: Obliczanie wierzchołków ---
    for (int i = 0; i <= density_R; ++i)
    {
        float u = i * step_R;

        for (int j = 0; j <= density_r; ++j)
        {
            float v = j * step_r;

            // Równania parametryczne torusa
            float x = (R + r * std::cos(v)) * std::cos(u);
            float y = (R + r * std::cos(v)) * std::sin(u);
            float z = r * std::sin(v);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // --- TOPOLOGIA: Łączenie wierzchołków w trójkąty ---
    for (int i = 0; i < density_R; ++i)
    {
        for (int j = 0; j < density_r; ++j)
        {
            int rowLength = density_r + 1;

            unsigned int p0 = i * rowLength + j;
            unsigned int p_right = (i + 1) * rowLength + j;     // Kolejny punkt na dużym okręgu
            unsigned int p_up = i * rowLength + (j + 1);        // Kolejny punkt na małym okręgu

            // Krawędź pozioma (wzdłuż dużego okręgu)
            indices.push_back(p0);
            indices.push_back(p_right);

            // Krawędź pionowa (wzdłuż małego przekroju)
            indices.push_back(p0);
            indices.push_back(p_up);
        }
    }
}
