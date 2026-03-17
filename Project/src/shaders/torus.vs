#version 460 core
layout (location = 0) in vec3 aPos;

// Macierze przekazywane z aplikacji (CPU)
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Mnożenie lewostronne: P * V * M * wektor_pozycji
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}