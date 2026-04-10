#pragma once
#include "sceneObject.h"
#include "MG1Math/Mat4.h"
#include <vector>

class ScenePoint : public SceneObject {
public:
    int selectedCurvesCount = 0;
    unsigned int VAO, VBO;
    float size;

    ScenePoint(std::string n, Transformations spawnTransform);
    ScenePoint(std::string n, float s, Transformations spawnTransform);
    ~ScenePoint() override;

    void Init() override;
    void Draw(Shader& shader) override;
    void Draw(Shader& shader, Mat4 parentMatrix) override;
};