#pragma once
#include "sceneObject.h"
#include "torusGrid.h"
#include "matrixesModelViewProjection.h"
#include "MG1Math/Mat4.h"
#include <vector>

class ScenePoint : public SceneObject {
public:
    unsigned int VAO, VBO;
    float color[3] = { 1.0f, 0.0f, 0.0f };

    ScenePoint(std::string n, Transformations spawnTransform);
    ~ScenePoint() override;

    void Init() override;
    void Draw(Shader& shader, Mat4 parentMatrix) override;
};