#pragma once

#include "holeManager.h"
#include "sceneGregoryPatch.h"

std::shared_ptr<SceneGregoryPatch> GenerateGregoryPatchForHole(
        const HoleCycle& hole
        //,std::vector<std::shared_ptr<SceneObject>>& sceneObjects
        );

void UpdateGregoryPositions(const std::weak_ptr<SceneGregoryPatch>& patch, const PreviewContext &ctx);