#pragma once
#include "screenInteractions.h"
#include "sceneBezierC0.h"


Vect3 getRayDirection(double mouseX, double mouseY, int winWidth, int winHeight, const Camera& camera)
{
    // [-1, 1]
    float ndcX = (2.0f * (float)mouseX) / (float)winWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * (float)mouseY) / (float)winHeight;

    // Pobieramy wszystkie 3 wektory JEDNYM superszybkim wywołaniem!
    Vect3 D(0.0), R(0.0), U(0.0);
    camera.getCameraVectors(D, R, U);

    float aspectRatio = (float)winWidth / (float)winHeight;
    float tanHalfFov = std::tan(camera.fov / 2.0f); // Używamy FOV z kamery!

    float viewX = ndcX * aspectRatio * tanHalfFov;
    float viewY = ndcY * tanHalfFov;

    Vect3 rayDir(
            R.x * viewX + U.x * viewY - D.x,
            R.y * viewX + U.y * viewY - D.y,
            R.z * viewX + U.z * viewY - D.z
    );

    return rayDir.normalize();
}

Vect3 getCursorIntersectionWithCameraPlane(Vect3 rayDir, const Camera& camera)
{
    Vect3 activePos(0.0), activeTarget(0.0);
    camera.getActiveState(activePos, activeTarget);

    Vect3 planeNormal = (activePos - activeTarget).normalize(); // Oś Z kamery
    float denominator = Vect3::dot(rayDir, planeNormal);

    if (std::abs(denominator) > 0.0001f)
    {
        Vect3 toTarget = activeTarget - activePos;
        float t = Vect3::dot(toTarget, planeNormal) / denominator;

        if (t > 0.0f)
        {
            return Vect3(activePos.x + rayDir.x * t,
                         activePos.y + rayDir.y * t,
                         activePos.z + rayDir.z * t);
        }
    }
    return activeTarget; // Punkt awaryjny
}


void handleSingleClickSelection(double mouseX, double mouseY, int winWidth, int winHeight,
                                       const Camera& camera, std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, camera);

    Vect3 activeCamPos(0.0), activeCamTarget(0.0);
    camera.getActiveState(activeCamPos, activeCamTarget);

    float minDist = 10000.0f;
    std::shared_ptr<SceneObject> closestObj = nullptr;

    for (auto& obj : sceneObjects)
    {
        // Obecnie klikamy tylko w punkty
        if(obj->objectType != ObjectType::Point)
            continue;


        Vect3 objPos = obj->transformations.getPosition();
        Vect3 toObj = objPos - activeCamPos;
        float projLength = Vect3::dot(toObj, rayDir);

        if (projLength > 0.0f)
        {
            Vect3 projPoint = activeCamPos + Vect3(rayDir.x * projLength, rayDir.y * projLength, rayDir.z * projLength);
            float distToRay = std::sqrt((objPos.x - projPoint.x) * (objPos.x - projPoint.x) +
                                        (objPos.y - projPoint.y) * (objPos.y - projPoint.y) +
                                        (objPos.z - projPoint.z) * (objPos.z - projPoint.z));

            // Promień trafienia (1.5f) - łapiemy ten najbliżej kamery
            if (distToRay < 1.5f && projLength < minDist)
            {
                minDist = projLength;
                closestObj = obj;
            }
        }
    }

    if (closestObj != nullptr)
        closestObj->isSelected = !closestObj->isSelected;
}



void performBoxSelection(double boxStartX, double boxStartY, double boxEndX, double boxEndY,
                                int winWidth, int winHeight, const Camera& camera,
                                std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    float aspectRatio = (float)winWidth / (float)winHeight;
    Mat4 M_View = camera.getViewMatrix();
    Mat4 M_Proj = camera.getProjectionMatrix(aspectRatio);
    Mat4 VP = M_Proj * M_View;

    float minX = (float)std::min(boxStartX, boxEndX);
    float maxX = (float)std::max(boxStartX, boxEndX);
    float minY = (float)std::min(boxStartY, boxEndY);
    float maxY = (float)std::max(boxStartY, boxEndY);


    for (auto& obj : sceneObjects)
    {
        if (obj->objectType == ObjectType::BezierCurveC0)
            continue;

        Vect3 pos = obj->transformations.getPosition();
        float screenX, screenY;

        // Magia! Jedno wywołanie ogarnia całą matematykę kamery i perspektywy
        if (projectWorldToScreen(pos, VP, winWidth, winHeight, screenX, screenY))
        {
            // Jeśli punkt jest przed kamerą, sprawdź czy leży w narysowanym prostokącie
            if (screenX >= minX && screenX <= maxX && screenY >= minY && screenY <= maxY)
            {
                obj->isSelected = true;
            }
        }
    }
}

