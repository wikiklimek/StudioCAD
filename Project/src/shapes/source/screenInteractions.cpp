#pragma once
#include "screenInteractions.h"

Vect3 getRayDirection(double mouseX, double mouseY, int winWidth, int winHeight,
                      Vect3 cameraPos, Vect3 target, Vect3 up, float fov)
{
    // 1. Normalizacja współrzędnych myszy na ekranie (NDC: od -1 do 1)
    float ndcX = (2.0f * (float)mouseX) / (float)winWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * (float)mouseY) / (float)winHeight; // Odwracamy Y, bo w oknie rośnie w dół

    // 2. Odtworzenie osi lokalnych kamery (dokładnie jak w Twoim createViewMatrix)
    Vect3 D = (cameraPos - target).normalize(); // Wektor "w tył" od celu do kamery
    Vect3 R = Vect3::cross(up, D).normalize();  // Wektor w prawo
    Vect3 U = Vect3::cross(D, R).normalize();   // Wektor w górę

    // 3. Przeliczenie na współrzędne widoku z uwzględnieniem perspektywy
    float aspectRatio = (float)winWidth / (float)winHeight;
    float tanHalfFov = std::tan(fov / 2.0f);

    float viewX = ndcX * aspectRatio * tanHalfFov;
    float viewY = ndcY * tanHalfFov;

    // 4. Transformacja do przestrzeni świata (kombinacja wektorów kierunkowych)
    Vect3 rayDir(
            R.x * viewX + U.x * viewY - D.x,
            R.y * viewX + U.y * viewY - D.y,
            R.z * viewX + U.z * viewY - D.z
    );

    return rayDir.normalize();
}

Vect3 getCursorIntersection(Vect3 rayOrigin, Vect3 rayDir)
{
    // Szukamy punktu przecięcia promienia z płaszczyzną Z = 0
    // Korzystamy z równania parametrycznego prostej: P = Origin + t * Direction

    // Zabezpieczenie przed dzieleniem przez zero (gdy patrzymy idealnie równolegle do podłogi)
    if (std::abs(rayDir.z) > 0.0001f) {
        float t = -rayOrigin.z / rayDir.z;

        // Jeśli t > 0, to punkt przecięcia znajduje się przed kamerą
        if (t > 0.0f) {
            return Vect3(rayOrigin.x + rayDir.x * t,
                         rayOrigin.y + rayDir.y * t,
                         0.0f);
        }
    }

    // Fallback: jeśli nie trafiamy w podłogę, zwracamy środek układu
    return Vect3(0.0f, 0.0f, 0.0f);
}