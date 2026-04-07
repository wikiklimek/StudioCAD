#pragma once
#include "transformManager.h"
#include "sceneBezierC0.h"
#include "bakeTransform.h"


// Wywoływane, gdy klikniesz na scenie, żeby zacząć ciągnąć
    void TransformManager::startTransformation(Vect3 centerOfSelection, Vect3 cursorPosition)
    {
        isTransformationActive = true;
        mouseDelta = Transformations();

        if (transformMode == COMMON_CENTER)
            centerOfTransformations = centerOfSelection;
        else if (transformMode == CURSOR_CENTER)
            centerOfTransformations = cursorPosition;
        else
            centerOfTransformations = Vect3(0.0f, 0.0f, 0.0f); // ENTIRE_SCENE / LOCAL
    }

    // Matematyka myszki w jednym miejscu!
    void TransformManager::processMouseDrag(float dx_world, float dy_world, const Camera& camera)
    {
        mouseDelta = Transformations(); // Zawsze nadpisujemy (nie dodajemy)

        if (currentMode == TRANSLATE)
        {
            Vect3 translation = calculateScreenSpaceTranslation(dx_world, dy_world, centerOfTransformations, camera);
            mouseDelta.posX = translation.x;
            mouseDelta.posY = translation.y;
            mouseDelta.posZ = translation.z;
        }
        else if (currentMode == ROTATE_X)
            mouseDelta.rotation = Quaternion::fromAxisAngle(1.0f, 0.0f, 0.0f, dy_world * 2.0f);
        else if (currentMode == ROTATE_Y)
            mouseDelta.rotation = Quaternion::fromAxisAngle(0.0f, 1.0f, 0.0f, dx_world * 2.0f);
        else if (currentMode == ROTATE_Z)
            mouseDelta.rotation = Quaternion::fromAxisAngle(0.0f, 0.0f, 1.0f, dx_world * 2.0f);
        else if (currentMode == SCALE)
            mouseDelta.scale = std::max(0.01f, 1.0f + (dx_world - dy_world) * 0.9f);
        else if (currentMode == ROTATE_FREE)
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
        if (distance < 0.1f) distance = 15.0f; // Awaryjny bezpiecznik

        float speed = distance * std::tan(camera.fov / 2.0f);

        return Vect3(
                (right.x * dx_world - localUp.x * dy_world) * speed,
                (right.y * dx_world - localUp.y * dy_world) * speed,
                (right.z * dx_world - localUp.z * dy_world) * speed
        );
    }

    // Wypiekanie myszki w jednym miejscu!
    void TransformManager::bakeMouseTransformations(std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
    {
        if (!isTransformationActive)
            return;

        if (transformMode == LOCAL)
        {
            for (auto& obj : sceneObjects)
            {
                if (obj->objectType == ObjectType::BezierCurveC0)
                {
                    continue;
                }

                bool shouldBake = obj->isSelected;
                if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj))
                {
                    if (p->selectedCurvesCount > 0)
                        shouldBake = true;
                }
                if (!shouldBake)
                    continue;

                obj->transformations.posX += mouseDelta.posX;
                obj->transformations.posY += mouseDelta.posY;
                obj->transformations.posZ += mouseDelta.posZ;
                obj->transformations.scale *= mouseDelta.scale;
                obj->transformations.rotation = mouseDelta.rotation * obj->transformations.rotation;
                obj->transformations.rotation.normalize();
            }
        }
        else if (transformMode == COMMON_CENTER || transformMode == CURSOR_CENTER || transformMode == ENTIRE_SCENE)
        {
            bakeGroupTransform(sceneObjects, mouseDelta, centerOfTransformations, transformMode == ENTIRE_SCENE);
        }

        mouseDelta = Transformations();
        isTransformationActive = false;
    }
