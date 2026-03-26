#pragma once
#include "sceneObject.h"
#include <vector>
#include <memory>
// Pamiętaj o upewnieniu się, że masz tu zaincludowane Vect3, Mat4 i Quaternion

void applyTransformationToSelected(std::vector<std::shared_ptr<SceneObject>>& objects,
                                   Vect3 pivot,
                                   Quaternion deltaQuat, // ZMIANA: Przyjmujemy czysty kwaternion
                                   float deltaScale)
{
    // Generujemy macierz obrotu bezpośrednio z kwaternionu
    // (używamy jej wyłącznie do obracania wektora pozycji względem pivota)
    Mat4 dRotMat = deltaQuat.toMat4();

    for (auto& obj : objects) {
        if (!obj->isSelected) continue;

        // 1. Obrót i skala orbity (zmiana pozycji na scenie)
        Vect3 vecToPivot = obj->transformations.getPosition() - pivot;
        vecToPivot = Vect3(vecToPivot.x * deltaScale, vecToPivot.y * deltaScale, vecToPivot.z * deltaScale);

        Vect4 v4(vecToPivot.x, vecToPivot.y, vecToPivot.z, 1.0f);
        Vect4 rotatedV4 = dRotMat * v4; // Obracamy wektor używając macierzy z kwaternionu
        Vect3 rotatedVec(rotatedV4.x, rotatedV4.y, rotatedV4.z);

        obj->transformations.setPosition(pivot + rotatedVec);
        obj->transformations.scale *= deltaScale;

        // 2. OBRÓT WŁASNY OBIEKTU
        // Składamy obroty poprzez mnożenie kwaternionów
        obj->transformations.rotation = deltaQuat * obj->transformations.rotation;

        // Zabezpieczenie przed akumulacją błędów numerycznych [cite: 40]
        obj->transformations.rotation.normalize();
    }
}