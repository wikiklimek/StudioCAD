#pragma once
#include <MG1Math/Vect3.h>
#include <memory>
#include <vector>

inline void bakeGroupTransform(std::vector<std::shared_ptr<SceneObject>>& objects, Transformations& groupTransform, Vect3 centerOfTransformations, bool applyToAll = false)
{
    Mat4 T_toOrigin = Mat4::translate_inverse(centerOfTransformations);
    Mat4 R_group = groupTransform.rotation.toMat4();
    Mat4 S_group = Mat4::scale(Vect3(groupTransform.scale, groupTransform.scale, groupTransform.scale));
    Mat4 T_toPos = Mat4::translate(centerOfTransformations + groupTransform.getPosition());

    Mat4 M_group = T_toPos * R_group * S_group * T_toOrigin;

    for(auto& obj : objects)
    {
        bool shouldBake = applyToAll || obj->isSelected;

        // Magiczna logika decyzyjna
        if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj)) {
            if (p->selectedCurvesCount > 0) shouldBake = true;
        }
        else if (obj->objectType == ObjectType::BezierCurveC0)
        {
            continue;
        }

        if(!shouldBake)
            continue;

        Vect3 oldPos = obj->transformations.getPosition();
        Vect4 pos4(oldPos.x, oldPos.y, oldPos.z, 1.0f);
        Vect4 newPos4 = M_group * pos4;
        obj->transformations.setPosition(Vect3(newPos4.x, newPos4.y, newPos4.z));

        obj->transformations.rotation = groupTransform.rotation * obj->transformations.rotation;
        obj->transformations.rotation.normalize();

        obj->transformations.scale *= groupTransform.scale;
    }
}