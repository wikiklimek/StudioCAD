#include "selectionManager.h"
#include "scenePoint.h"
#include "sceneBezierC2.h"


void unselectVirtualPointsAndActualizePointsSelectedBeziers(std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    for(std::shared_ptr<SceneObject>& obj : sceneObjects)
    {
        if (obj->objectType == ObjectType::Point)
        {
            auto p = std::static_pointer_cast<ScenePoint>(obj);
            p->isSelectedAsDeBoore = false;
            p->virtualWeight = 0.0f;
        }

        if (obj->objectType == ObjectType::BezierCurveC2)
        {
            auto b2 = std::static_pointer_cast<SceneBezierC2>(obj);
            for (auto &vp: b2->virtualPoints)
                vp->isSelected = false;
        }

        //zmianiamy selection bezrerów punktów
        if (obj->objectType == ObjectType::BezierCurveC0 ||
            obj->objectType == ObjectType::BezierCurveC2 ||
            obj->objectType == ObjectType::SplineInterpolating)
        {
            auto b = std::static_pointer_cast<SceneBezier>(obj);

            if (b->wasGuiSelectionChanged)
            {
                if(b->isSelected)
                    for (auto& wp : b->points)
                    {
                        if (auto p = wp.lock())
                            p->selectedCurvesCount++;
                    }
                else
                    for (auto& wp : b->points)
                    {
                        if (auto p = wp.lock())
                            p->selectedCurvesCount--;
                    }
            }
        }
    }
}

void unselectObjectsAndVirtualPointsAndCleanPointsSelectedBeziers(std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    for(auto& obj : sceneObjects)
    {
        obj->isSelected = false;

        if (obj->objectType == ObjectType::Point)
        {
            auto p = std::static_pointer_cast<ScenePoint>(obj);

            //do wirtualnych punktów
            p->isSelectedAsDeBoore = false;
            p->virtualWeight = 0.0f;

            //do krzywych beziera
            p->selectedCurvesCount = 0;
        }

        if (obj->objectType == ObjectType::BezierCurveC2)
        {
            auto b2 = std::static_pointer_cast<SceneBezierC2>(obj);
            for (auto &vp: b2->virtualPoints)
                vp->isSelected = false;
        }
    }
}
