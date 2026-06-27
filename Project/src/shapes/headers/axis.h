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

    void Draw(Shader& shader, const Vect3& position, const Vect3& rotations, float length);

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
};