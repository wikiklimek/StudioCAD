#pragma once
#include "screenInteractions.h"

Vect3 getRayDirection(double mouseX, double mouseY, int winWidth, int winHeight,
                      Vect3 cameraPos, Vect3 target, Vect3 up, float fov)
{
    // [-1, 1]
    float ndcX = (2.0f * (float)mouseX) / (float)winWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * (float)mouseY) / (float)winHeight;

    Vect3 D = (cameraPos - target).normalize(); // wektor w tył
    Vect3 R = Vect3::cross(up, D).normalize();
    Vect3 U = Vect3::cross(D, R).normalize();

    float aspectRatio = (float)winWidth / (float)winHeight;
    float tanHalfFov = std::tan(fov / 2.0f);

    float viewX = ndcX * aspectRatio * tanHalfFov;
    float viewY = ndcY * tanHalfFov;

    Vect3 rayDir(
            R.x * viewX + U.x * viewY - D.x,
            R.y * viewX + U.y * viewY - D.y,
            R.z * viewX + U.z * viewY - D.z
    );

    return rayDir.normalize();
}

Vect3 getCursorIntersection(Vect3 rayOrigin, Vect3 rayDir)
{
    if (std::abs(rayDir.z) > 0.0001f)
    {
        // stawiamy obiekty w z = 0
        float z = 0.0f;
        // z = rayOrigin.z + t * rayDir.z
        float t = (z -rayOrigin.z) / rayDir.z;

        // czy widzimy intersection
        if (t > 0.0f)
        {
            return Vect3(rayOrigin.x + rayDir.x * t,
                         rayOrigin.y + rayDir.y * t,
                         0.0f);
        }
    }


    return Vect3(0.0f, 0.0f, 0.0f);
}