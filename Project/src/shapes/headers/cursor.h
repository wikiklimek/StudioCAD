#pragma once
#include "sceneObject.h"
#include "torusGrid.h"
#include "matrixesModelViewProjection.h"
#include "MG1Math/Mat4.h"
#include <vector>
#include <memory>


class Cursor {
public:
    Transformations transform;
    float screenX = 0.0f, screenY = 0.0f;
    unsigned int VAO=0, VBO=0;


    ~Cursor();

    void Init();


    void Draw(Shader& shader);
};
