#pragma once
#include "screenInteractions.h"
#include "sceneBezierC0.h"


Vect3 getRayDirection(double mouseX, double mouseY, int winWidth, int winHeight, const Camera& camera)
{
    // [-1, 1]
    float ndcX = (2.0f * (float)mouseX) / (float)winWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * (float)mouseY) / (float)winHeight;

    Vect3 D(0.0), R(0.0), U(0.0);
    camera.getCameraVectors(D, R, U);

    float aspectRatio = (float)winWidth / (float)winHeight;
    float tanHalfFov = std::tan(camera.fov / 2.0f);

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


bool handleSingleClickSelection(double mouseX, double mouseY, int winWidth, int winHeight,
                                const Camera& camera, std::vector<std::shared_ptr<SceneObject>>& sceneObjects,
                                std::shared_ptr<SceneBezierC2> & closestVirtualPointBezierOwner)
{
    bool isSelectedClosestVirtualPointBezierOwner = false;

    float aspectRatio = (float)winWidth / (float)winHeight;
    Mat4 VP = camera.getProjectionMatrix(aspectRatio) * camera.getViewMatrix();

    Vect3 activeCamPos(0.0), activeCamTarget(0.0);
    camera.getActiveState(activeCamPos, activeCamTarget);

    float minDistToCamera = 100000.0f;
    std::shared_ptr<ScenePoint> closestPoint = nullptr;


    auto isPointCloserToClick = [&VP, &winWidth, &winHeight, &mouseX,
            &mouseY, &activeCamPos, &closestPoint, &minDistToCamera] (std::shared_ptr<ScenePoint>& p)
    {
        Vect3 pos = p->transformations.getPosition();

        float screenX, screenY;

        if (projectWorldToScreen(pos, VP, winWidth, winHeight, screenX, screenY))
        {
            float dx = (float)mouseX - screenX;
            float dy = (float)mouseY - screenY;
            float dist2D = std::sqrt(dx*dx + dy*dy);

            float clickRadius = (p->size / 2.0f) + 4.0f;

            if (dist2D <= clickRadius)
            {
                float dist3D = (pos.x - activeCamPos.x) * (pos.x - activeCamPos.x) +
                               (pos.y - activeCamPos.y) * (pos.y - activeCamPos.y) +
                               (pos.z - activeCamPos.z) * (pos.z - activeCamPos.z);

                if (dist3D < minDistToCamera)
                {
                    minDistToCamera = dist3D;
                    closestPoint = p;

                    return true;
                }
            }
        }

        return false;
    };


    // --- wszystkie punkty (Zwykłe + Wirtualne) ---
    for (auto& obj : sceneObjects)
    {
        // Zbierz normalne punkty ze sceny
        if (obj->objectType == ObjectType::Point)
        {
            auto p = std::static_pointer_cast<ScenePoint>(obj);
            if(isPointCloserToClick(p))
            {
                closestVirtualPointBezierOwner = nullptr;
                isSelectedClosestVirtualPointBezierOwner = false;
            }
        }
        // Zajrzyj do kieszeni krzywych C2
        else if (obj->objectType == ObjectType::BezierCurveC2)
        {
            auto b2 = std::static_pointer_cast<SceneBezierC2>(obj);
            if (b2->currentBasis == BezierBasisMode::BERNSTEIN)
            {
                for (auto& vp : b2->virtualPoints)
                {
                    if(isPointCloserToClick(vp))
                    {
                        closestVirtualPointBezierOwner = b2;
                        isSelectedClosestVirtualPointBezierOwner = true;
                    }
                }
            }
        }
    }


    if (closestPoint != nullptr)
    {
        closestPoint->isSelected = true; //!closestPoint->isSelected;
    }


    return isSelectedClosestVirtualPointBezierOwner;
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

    //  Agregacja wszystkich zaznaczalnych obiektów
    std::vector<std::shared_ptr<SceneObject>> selectableObjects;
    for (auto& obj : sceneObjects)
    {
        // Bezpośrednio zaznaczalne obiekty (Torusy, standardowe Punkty)
        if (obj->objectType == ObjectType::Point || obj->objectType == ObjectType::Torus)
        {
            selectableObjects.push_back(obj);
        }


        // Krzywe Beziera C0 i same obiekty C2 ignorujemy
        // nie chcemy tez box selectowa punktów wirtualnych z BezierC2 bo by sie logika rozjechała
    }

    // Sprawdzenie kolizji Boxa z wyciągniętymi obiektami
    for (auto& obj : selectableObjects)
    {
        Vect3 pos = obj->transformations.getPosition();
        float screenX, screenY;

        if (projectWorldToScreen(pos, VP, winWidth, winHeight, screenX, screenY))
        {
            if (screenX >= minX && screenX <= maxX && screenY >= minY && screenY <= maxY)
            {
                obj->isSelected = true;
            }
        }
    }
}
