#pragma once
#include "camera.h"
#include "sceneObject.h"
#include "scenePoint.h"
#include <iostream>
#include <memory>


Vect3 getRayDirection(double mouseX, double mouseY, int winWidth, int winHeight, const Camera& camera);

Vect3 getCursorIntersectionWithCameraPlane(Vect3 rayDir, const Camera& camera);


void handleSingleClickSelection(double mouseX, double mouseY, int winWidth, int winHeight,
                                       const Camera& camera, std::vector<std::shared_ptr<SceneObject>>& sceneObjects);



void performBoxSelection(double boxStartX, double boxStartY, double boxEndX, double boxEndY,
                         int winWidth, int winHeight, const Camera& camera,
                         std::vector<std::shared_ptr<SceneObject>>& sceneObjects);