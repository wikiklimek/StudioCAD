#pragma once
#include "sceneObject.h"
#include <vector>
#include <memory>
// Pamiętaj o upewnieniu się, że masz tu zaincludowane Vect3, Mat4 i Quaternion

void applyTransformationToSelected(std::vector<std::shared_ptr<SceneObject>>& objects,
                                   Vect3 pivot,
                                   Quaternion deltaQuat,
                                   float deltaScale)
{
    //Mat4 dRotMat = deltaQuat.toMat4();

    //for (auto& obj : objects) {
        //if (!obj->isSelected) continue;

        //Vect3 vecToPivot = obj->transformations.getPosition() - pivot;
        //vecToPivot = Vect3(vecToPivot.x * deltaScale, vecToPivot.y * deltaScale, vecToPivot.z * deltaScale);

        //Vect4 v4(vecToPivot.x, vecToPivot.y, vecToPivot.z, 1.0f);
        //Vect4 rotatedV4 = dRotMat * v4; // Obracamy wektor używając macierzy z kwaternionu
        //Vect3 rotatedVec(rotatedV4.x, rotatedV4.y, rotatedV4.z);

        //obj->transformations.setPosition(pivot + rotatedVec);
        //obj->transformations.scale *= deltaScale;

        //obj->transformations.rotation = deltaQuat * obj->transformations.rotation;

        //obj->transformations.rotation.normalize();
    //}
}