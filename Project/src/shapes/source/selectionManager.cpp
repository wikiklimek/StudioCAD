#include "selectionManager.h"
#include "scenePoint.h"
#include "sceneBezierC2.h"
#include "sceneSurface.h"


void unselectVirtualPointsAndActualizePointsSelectedBeziers(std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    //ta funkcja musi byc po operacjach z gui

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

        if (obj->objectType == ObjectType::BezierSurfaceC0 ||
            obj->objectType == ObjectType::BezierSurfaceC2)
        {
            auto s = std::static_pointer_cast<SceneSurface>(obj);
            if (s->wasGuiSelectionChanged)
            {
                if(s->isSelected)
                    for (auto& wp : s->points)
                    {
                        if (auto p = wp.lock())
                            p->selectedSurfacesCount++;
                    }
                else
                    for (auto& wp : s->points)
                    {
                        if (auto p = wp.lock())
                            p->selectedSurfacesCount--;
                    }
            }
        }
    }
}

void unselectObjectsAndVirtualPointsAndCleanPointsSelectedBeziers(std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    //ta operacja musi byc w moemncie wypiekania select boxa

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

            //do płatów
            p->selectedSurfacesCount = 0;
        }

        if (obj->objectType == ObjectType::BezierCurveC2)
        {
            auto b2 = std::static_pointer_cast<SceneBezierC2>(obj);
            for (auto &vp: b2->virtualPoints)
                vp->isSelected = false;
        }
    }
}
