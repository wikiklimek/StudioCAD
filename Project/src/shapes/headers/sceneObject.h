#pragma once
#include <utility>

#include "shader.h"
#include "transformations.h"
#include "enums.h"

class SceneObject {
private :
    float color_to_draw[3] = {0.7f, 0.7f, 0.0f };

protected:
    float* getUpdatedColorToDrawBasedOn(bool condition_selected)
    {
        if(condition_selected)
        {
            color_to_draw[0] = 1.0f;
            color_to_draw[1] = 0.0f;
            color_to_draw[2] = 0.0f;
        }
        else
        {
            color_to_draw[0] = color[0] * 0.7f;
            color_to_draw[1] = color[1] * 0.7f;
            color_to_draw[2] = color[2] * 0.7f;
        }
        return color_to_draw;
    }

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
    virtual float* getUpdatedColorToDraw()
    {
        return getUpdatedColorToDrawBasedOn(isSelected);
    }

};