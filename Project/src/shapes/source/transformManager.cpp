#pragma once
#include "transformManager.h"
#include "sceneBezierC0.h"
#include "bakeTransform.h"


// klikniecie na scene - rozpoczecie przeciagania
void TransformManager::startTransformation(Vect3 centerOfSelection, Vect3 cursorPosition, const AppState& state)
{
    isTransformationActive = true;
    mouseDelta = Transformations();

    if (state.transformMode == COMMON_CENTER)
        centerOfTransformations = centerOfSelection;
    else if (state.transformMode == CURSOR_CENTER)
        centerOfTransformations = cursorPosition;
    else
        centerOfTransformations = Vect3(0.0f, 0.0f, 0.0f);
}


void TransformManager::processMouseDrag(float dx_world, float dy_world, const Camera& camera, const AppState& state)
{
    mouseDelta = Transformations();

    if (state.currentMode == TRANSLATE)
    {
        Vect3 translation = calculateScreenSpaceTranslation(dx_world, dy_world, centerOfTransformations, camera);
        mouseDelta.posX = translation.x;
        mouseDelta.posY = translation.y;
        mouseDelta.posZ = translation.z;
    }
    else if (state.currentMode == ROTATE_X)
        mouseDelta.rotation = Quaternion::fromAxisAngle(1.0f, 0.0f, 0.0f, dy_world * 2.0f);
    else if (state.currentMode == ROTATE_Y)
        mouseDelta.rotation = Quaternion::fromAxisAngle(0.0f, 1.0f, 0.0f, dx_world * 2.0f);
    else if (state.currentMode == ROTATE_Z)
        mouseDelta.rotation = Quaternion::fromAxisAngle(0.0f, 0.0f, 1.0f, dx_world * 2.0f);
    else if (state.currentMode == SCALE)
        mouseDelta.scale = std::max(0.01f, 1.0f + (dx_world - dy_world) * 0.9f);
    else if (state.currentMode == ROTATE_FREE)
    {
        float angle = std::sqrt(dx_world * dx_world + dy_world * dy_world) * 2.0f;
        if (angle > 0.0001f)
            mouseDelta.rotation = Quaternion::fromAxisAngle(dy_world, dx_world, 0.0f, angle);
    }
}

Vect3 calculateScreenSpaceTranslation(float dx_world, float dy_world, Vect3 centerOfTransformations, const Camera& camera)
{
    Vect3 forward(0.0f), right(0.0f), localUp(0.0f);
    camera.getCameraVectors(forward, right, localUp);

    Vect3 activeCamPos(0.0), activeCamTarget(0.0);
    camera.getActiveState(activeCamPos, activeCamTarget);

    Vect3 toCenter = centerOfTransformations - activeCamPos;
    float distance = std::sqrt(toCenter.x*toCenter.x + toCenter.y*toCenter.y + toCenter.z*toCenter.z);
    if (distance < 0.1f)
        distance = 15.0f;

    float speed = distance * std::tan(camera.fov / 2.0f);

    return Vect3(
            (right.x * dx_world - localUp.x * dy_world) * speed,
            (right.y * dx_world - localUp.y * dy_world) * speed,
            (right.z * dx_world - localUp.z * dy_world) * speed
            );
}


void TransformManager::bakeMouseTransformations(std::vector<std::shared_ptr<SceneObject>>& sceneObjects, const AppState& state)
{
    if (!isTransformationActive)
        return;

    bakeTransformations(sceneObjects, mouseDelta, state.transformMode, centerOfTransformations);

    mouseDelta = Transformations();
    isTransformationActive = false;
}
