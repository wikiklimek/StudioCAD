#pragma once

#include "holeManager.h"
#include "sceneGregoryPatch.h"

std::shared_ptr<SceneGregoryPatch> GenerateGregoryPatchForHole(
        const HoleCycle& hole, std::vector<std::shared_ptr<SceneObject>>& sceneObjects);