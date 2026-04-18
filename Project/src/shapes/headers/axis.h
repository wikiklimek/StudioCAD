#pragma once
#include <glad/glad.h>
#include <MG1Math/Vect3.h>
#include <MG1Math/Mat4.h>
#include "shader.h"

class Axis {
public:
    Axis();
    ~Axis();

    void Init();

    // Używamy Vect3 dla spójności z resztą silnika!
    void Draw(Shader& shader, const Vect3& position, const Vect3& rotations, float length);

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
};