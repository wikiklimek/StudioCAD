#pragma once
#include <MG1Math/Vect3.h>
#include <memory>
#include <vector>
#include "transformations.h"
#include "sceneObject.h"

void bakeGroupTransform(std::vector<std::shared_ptr<SceneObject>>& objects, Transformations& groupTransform, Vect3 pivotCenter, bool applyToAll = false) {
    // Krok 1: Tworzymy macierz transformacji dla całej grupy wokół pivota
    Mat4 T_toOrigin = Mat4::translate_inverse(pivotCenter);
    Mat4 R_group = groupTransform.rotation.toMat4();
    Mat4 S_group = Mat4::scale(Vect3(groupTransform.scale, groupTransform.scale, groupTransform.scale));
    Mat4 T_toPos = Mat4::translate(pivotCenter + groupTransform.getPosition());

    Mat4 M_group = T_toPos * R_group * S_group * T_toOrigin;

    for(auto& obj : objects) {
        // Jeśli obiekt nie jest zaznaczony ORAZ nie aplikujemy do wszystkich, pomiń
        if(!obj->isSelected && !applyToAll) continue;

        // A) Wypiekamy nową pozycję
        Vect3 oldPos = obj->transformations.getPosition();
        Vect4 pos4(oldPos.x, oldPos.y, oldPos.z, 1.0f);
        Vect4 newPos4 = M_group * pos4;
        obj->transformations.setPosition(Vect3(newPos4.x, newPos4.y, newPos4.z));

        // B) Wypiekamy nową rotację
        obj->transformations.rotation = groupTransform.rotation * obj->transformations.rotation;
        obj->transformations.rotation.normalize();

        // C) Wypiekamy nową skalę
        obj->transformations.scale *= groupTransform.scale;
    }

    // Krok 2: Zerujemy pomocniczą strukturę
    groupTransform = Transformations();
}