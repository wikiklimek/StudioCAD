#pragma once
#include <iostream>


#include "grid.h"


void generateGrid(float gridHalfSize, std::vector<float>& gridVertices)
{
    for (int i = -gridHalfSize; i <= gridHalfSize; ++i)
    {
        gridVertices.push_back((float) i);
        gridVertices.push_back((float) -gridHalfSize);
        gridVertices.push_back(0.0f);
        gridVertices.push_back((float) i);
        gridVertices.push_back((float) gridHalfSize);
        gridVertices.push_back(0.0f);

        gridVertices.push_back((float) -gridHalfSize);
        gridVertices.push_back((float) i);
        gridVertices.push_back(0.0f);
        gridVertices.push_back((float) gridHalfSize);
        gridVertices.push_back((float) i);
        gridVertices.push_back(0.0f);
    }
}