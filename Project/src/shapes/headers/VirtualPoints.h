#pragma once
#include "sceneObject.h"
#include "scenePoint.h"
#include "shader.h"
#include "previewContext.h"
#include <vector>
#include <memory>
#include <string>


class VirtualPoints
{
public:
    std::vector<std::shared_ptr<ScenePoint>> virtualPoints;
    virtual void UpdateVirtualPointsIfNeeded(const PreviewContext& ctx) = 0;

};
