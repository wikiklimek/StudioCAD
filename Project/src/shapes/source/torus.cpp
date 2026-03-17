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
        float phi = i * step_R; // Kąt po dużym okręgu (wokół głównej osi torusa)

        for (int j = 0; j <= density_r; ++j)
        {
            float theta = j * step_r; // Kąt po małym okręgu (przekrój rury)

            // Równania parametryczne torusa
            float x = (R + r * std::cos(theta)) * std::cos(phi);
            float y = (R + r * std::cos(theta)) * std::sin(phi);
            float z = r * std::sin(theta);

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
            // Jeden pełny obwód małego przekroju ma density_r + 1 wierzchołków
            int rowLength = density_r + 1;

            // Obliczanie indeksów 4 wierzchołków tworzących "kwadrat" na powierzchni torusa
            unsigned int p0 = i * rowLength + j;
            unsigned int p1 = (i + 1) * rowLength + j;
            unsigned int p2 = (i + 1) * rowLength + (j + 1);
            unsigned int p3 = i * rowLength + (j + 1);

            // Dzielimy kwadrat na dwa trójkąty:
            indices.push_back(p0);
            indices.push_back(p1);
            indices.push_back(p2);

            indices.push_back(p0);
            indices.push_back(p2);
            indices.push_back(p3);
        }
    }
}
