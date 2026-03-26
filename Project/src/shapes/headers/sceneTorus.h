#pragma once
#include "sceneObject.h"
#include "torusGrid.h"
#include "matrixesModelViewProjection.h"
#include "MG1Math/Mat4.h"
#include <vector>

class SceneTorus : public SceneObject {
public:
    float R = 3.0f, r = 1.0f;
    int density_R = 30, density_r = 15;
    float color[3] = { 1.0f, 1.0f, 0.0f };

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    SceneTorus(std::string n, Transformations spawnTransform);
    ~SceneTorus() override;

    void Init() override ;
    void UpdateBuffers() ;
    void Draw(Shader& shader, Mat4 parentMatrix) override;
};
