#pragma once
#include "camera.h"
#include "sceneObject.h"
#include "scenePoint.h"
#include "sceneBezierC2.h"
#include <iostream>
#include <memory>


inline bool projectWorldToScreen(Vect3 worldPos, const Mat4& VP, int winWidth, int winHeight, float& outScreenX, float& outScreenY)
{
    Vect4 p4(worldPos.x, worldPos.y, worldPos.z, 1.0f);
    Vect4 clip = VP * p4;

    // za kamerą lub w samym oku kamery
    if (clip.w < 0.0001f)
        return false;

    // kostka [-1, 1]
    float nx = clip.x / clip.w;
    float ny = clip.y / clip.w;

    outScreenX = (nx + 1.0f) / 2.0f * (float)winWidth;
    outScreenY = (1.0f - ny) / 2.0f * (float)winHeight;

    return true;
}




Vect3 getRayDirection(double mouseX, double mouseY, int winWidth, int winHeight, const Camera& camera);

Vect3 getCursorIntersectionWithCameraPlane(Vect3 rayDir, const Camera& camera);


std::shared_ptr<SceneBezierC2> handleSingleClickSelection(double mouseX, double mouseY, int winWidth, int winHeight,
                                       const Camera& camera, std::vector<std::shared_ptr<SceneObject>>& sceneObjects);



void performBoxSelection(double boxStartX, double boxStartY, double boxEndX, double boxEndY,
                         int winWidth, int winHeight, const Camera& camera,
                         std::vector<std::shared_ptr<SceneObject>>& sceneObjects);