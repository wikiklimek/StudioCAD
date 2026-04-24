#pragma once
#include <MG1Math/Vect3.h>
#include <memory>
#include <vector>



inline void bakeGroupTransform(std::vector<std::shared_ptr<SceneObject>>& objects, const Transformations& groupTransform, Vect3 centerOfTransformations)
{
    Mat4 T_toOrigin = Mat4::translate_inverse(centerOfTransformations);
    Mat4 R_group = groupTransform.rotation.toMat4();
    Mat4 S_group = Mat4::scale(Vect3(groupTransform.scale, groupTransform.scale, groupTransform.scale));
    Mat4 T_toPos = Mat4::translate(centerOfTransformations + groupTransform.getPosition());

    Mat4 M_group = T_toPos * R_group * S_group * T_toOrigin;

    for(auto& obj : objects)
    {
        if (obj->objectType == ObjectType::BezierCurveC0 ||
        obj->objectType == ObjectType::BezierCurveC2 ||
        obj->objectType == ObjectType::SplineInterpolating)
            continue; // nie transformuje sie krzywej

        bool shouldBake = obj->isSelected;
        bool asDeBoor = false;
        float vWeight = 1.0f;
        if (obj->objectType == ObjectType::Point)
        {
            auto p = std::static_pointer_cast<ScenePoint>(obj);
            if (p->selectedCurvesCount > 0)
                shouldBake = true;
            asDeBoor = p->isSelectedAsDeBoore;
            vWeight = p->virtualWeight;
        }

        if(!shouldBake && !asDeBoor)
            continue;

        Vect3 oldPos = obj->transformations.getPosition();
        Vect4 pos4(oldPos.x, oldPos.y, oldPos.z, 1.0f);
        Vect4 newPos4 = M_group * pos4;
        Vect3 deltaPos = Vect3(newPos4.x, newPos4.y, newPos4.z) - oldPos;

        if (asDeBoor)
        {
            obj->transformations.setPosition(oldPos + deltaPos * vWeight);
        }
        else
        {
            obj->transformations.setPosition(oldPos + deltaPos);
            obj->transformations.rotation = groupTransform.rotation * obj->transformations.rotation;
            obj->transformations.rotation.normalize();
            obj->transformations.scale *= groupTransform.scale;
        }
    }
}

//bardzo ważne że bakeTransformation działa niezaleznie od myszka/GUI
inline void bakeTransformations(std::vector<std::shared_ptr<SceneObject>>& sceneObjects, const Transformations& delta, TransformMode mode, Vect3 centerOfTransformations)
{
    if (mode == LOCAL)
    {
        for (auto& obj : sceneObjects)
        {
            if (obj->objectType == ObjectType::BezierCurveC0 ||
            obj->objectType == ObjectType::BezierCurveC2 ||
            obj->objectType == ObjectType::SplineInterpolating)
                continue;

            bool shouldBake = obj->isSelected;
            bool asDeBoor = false;
            float vWeight = 1.0f;
            if (obj->objectType == ObjectType::Point)
            {
                auto p = std::static_pointer_cast<ScenePoint>(obj);
                if (p->selectedCurvesCount > 0)
                    shouldBake = true;

                asDeBoor = p->isSelectedAsDeBoore;
                vWeight = p->virtualWeight;
            }

            if (shouldBake || asDeBoor)
            {
                float multiplier = asDeBoor ? vWeight : 1.0f;
                obj->transformations.posX += delta.posX * multiplier;
                obj->transformations.posY += delta.posY * multiplier;
                obj->transformations.posZ += delta.posZ * multiplier;

                if (shouldBake)
                {
                    obj->transformations.scale *= delta.scale;
                    obj->transformations.rotation = delta.rotation * obj->transformations.rotation;
                    obj->transformations.rotation.normalize();
                }
            }
        }
    }
    else
    {
        bakeGroupTransform(sceneObjects, delta, centerOfTransformations);
    }
}