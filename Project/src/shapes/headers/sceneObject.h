#pragma once
#include <utility>

#include "shader.h"
#include "transformations.h"

class SceneObject {
public:
    Transformations transformations;
    std::string name;
    bool isSelected = false;

    SceneObject(std::string n, Transformations spawnTransform) : name(std::move(n)), transformations(spawnTransform) {}
    virtual ~SceneObject() = default;

    virtual void Init() = 0;
    virtual void Draw(Shader& shader, Mat4 parentMatrix) = 0;
};