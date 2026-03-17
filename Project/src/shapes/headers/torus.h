#pragma once
#include <iostream>
#include <vector>
// Nagłówek funkcji generującej torus
void generateTorus(float R, float r,  int density_R, int density_r,  std::vector<float>& vertices, std::vector<unsigned int>& indices);
