#include "deleteManager.h"
#include "sceneGregoryPatch.h"

void deleteObjects(GuiManager& guiManager, std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    
    if (guiManager.deleteSelectedPressed)
    {
        for (auto& obj : sceneObjects)
        {
            if (obj->isSelected)
            {
                if (obj->objectType != ObjectType::Point)
                {
                    obj->pendingDelete = true;
                }
            }
        }
    }

    for (auto& obj : sceneObjects)
    {
        if (obj->pendingDelete)
        {

            if (obj->objectType == ObjectType::BezierCurveC0 ||
                obj->objectType == ObjectType::BezierCurveC2 ||
                obj->objectType == ObjectType::SplineInterpolating)
            {
                auto b = std::static_pointer_cast<SceneBezier>(obj);
                for (auto& wp : b->points)
                {
                    if (auto p = wp.lock())
                        p->globalCurvesCount--;
                }
            }
            else if (obj->objectType == ObjectType::BezierSurfaceC0 ||
                    obj->objectType == ObjectType::BezierSurfaceC2)
            {
                auto s = std::static_pointer_cast<SceneSurface>(obj);
                for (auto& wp : s->points)
                {
                    if (auto p = wp.lock())
                        p->globalSurfacesCount--;
                }
            }
            else if (obj->objectType == ObjectType::GregoryPatch)
            {
                auto g = std::static_pointer_cast<SceneGregoryPatch>(obj);
                for (auto& wp : g->bezierPatchPoints)
                {
                    if (auto p = wp.lock())
                    {
                        p->globalSurfacesCount--;
                    }
                }
                for (auto& wp : g->bezierPatchPointsInner)
                {
                    if (auto p = wp.lock())
                    {
                        p->globalSurfacesCount--;
                    }
                }
            }
        }
    }


    for (auto& obj : sceneObjects)
    {
        if (obj->pendingDelete &&
            (obj->objectType == ObjectType::BezierSurfaceC0 ||
            obj->objectType == ObjectType::BezierSurfaceC2))
        {
            auto s = std::static_pointer_cast<SceneSurface>(obj);
            for (auto& wp : s->points)
            {
                if (auto p = wp.lock())
                {
                    if (guiManager.surfaceDeletionMode == 1 || guiManager.surfaceDeletionMode == 2)
                    {
                        if (p->globalCurvesCount == 0 && p->globalSurfacesCount == 0)
                            p->pendingDelete = true;
                    }
                }
            }
        }
        else if (obj->pendingDelete &&
            obj->objectType == ObjectType::GregoryPatch) //zastanów sie czy chcesz aby to tak działało
        {
            auto s = std::static_pointer_cast<SceneGregoryPatch>(obj);
            for (auto& wp : s->bezierPatchPoints)
            {
                if (auto p = wp.lock())
                {
                    if (guiManager.surfaceDeletionMode == 1 || guiManager.surfaceDeletionMode == 2)
                    {
                        if (p->globalCurvesCount == 0 && p->globalSurfacesCount == 0)
                            p->pendingDelete = true;
                    }
                }
            }
            for (auto& wp : s->bezierPatchPointsInner)
            {
                if (auto p = wp.lock())
                {
                    if (guiManager.surfaceDeletionMode == 1 || guiManager.surfaceDeletionMode == 2)
                    {
                        if (p->globalCurvesCount == 0 && p->globalSurfacesCount == 0)
                            p->pendingDelete = true;
                    }
                }
            }
        }
    }

    if (guiManager.deleteSelectedPressed)
    {
        for (auto& obj : sceneObjects)
        {
            if (obj->objectType == ObjectType::Point && obj->isSelected)
            {
                auto p = std::static_pointer_cast<ScenePoint>(obj);
                if (p->globalSurfacesCount == 0)
                {
                    p->pendingDelete = true;
                }
            }
        }
    }


    bool anyObjectDeleted = false;
    for (const auto& obj : sceneObjects)
    {
        if (obj->pendingDelete)
        {
            anyObjectDeleted = true;
            break;
        }
    }

    if (anyObjectDeleted)
    {
        guiManager.holesPotentialChanges = true; 
    }


    sceneObjects.erase(std::remove_if(sceneObjects.begin(), sceneObjects.end(),
                                      [](const std::shared_ptr<SceneObject>& o) { return o->pendingDelete; }), sceneObjects.end());

}
