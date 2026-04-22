#pragma once
#include <utility>

#include "shader.h"
#include "transformations.h"
#include "enums.h"

class SceneObject {
public:
    const ObjectType objectType;

    Transformations transformations;
    std::string name;
    bool isSelected = false;
    bool wasGuiSelectionChanged = false; //aktywne tylko 1 klatke - jak zmieniliśmy select
    bool pendingDelete = false;
    float color[3] = {1.0f, 1.0f, 0.0f };

    SceneObject(std::string n, Transformations spawnTransform, ObjectType type)
            : name(std::move(n)), transformations(spawnTransform), objectType(type) {}
    virtual ~SceneObject() = default;

    virtual void Init() = 0;
    virtual void Draw(Shader& shader) = 0;
    virtual void Draw(Shader& shader, Mat4 parentMatrix) = 0;
};