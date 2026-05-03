#pragma once
#include <vector>
#include <memory>
#include "sceneObject.h"

void unselectVirtualPointsAndActualizePointsSelectedBeziers(std::vector<std::shared_ptr<SceneObject>>& sceneObjects);
void unselectObjectsAndVirtualPointsAndCleanPointsSelectedBeziers(std::vector<std::shared_ptr<SceneObject>>& sceneObjects);
