#pragma once
#include "sceneObject.h"
#include "torusGrid.h"
#include "matrixesModelViewProjection.h"
#include "MG1Math/Mat4.h"
#include <vector>
#include <memory>

void applyTransformationToSelected(std::vector<std::shared_ptr<SceneObject>>& objects,
                                   Vect3 pivot,
                                   Quaternion deltaQuat,
                                   float deltaScale);