#include "deleteManager.h"
#include "sceneGregoryPatch.h"

void deleteObjects(GuiManager& guiManager, std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    // zbieranie akcji usunięcia przyciskiem usuwajacym wszytsie zaznaczone obioekty
    if (guiManager.deleteSelectedPressed)
    {
        for (auto& obj : sceneObjects)
        {
            if (obj->isSelected)
            {
                // punkty pomijamy bo musimy najpierw usunąc im powiązania z usuwanymi krzywymi/płasczyznami
                if (obj->objectType != ObjectType::Point)
                {
                    obj->pendingDelete = true;
                }
            }
        }
    }

    // czyszczenie powiązań punktów, najpier tylko z krzywymi
    // dla każdego pending delete - prywatnego i globalnego
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
                // ============================================
                // NOWE: Zdejmowanie liczników Płata Gregory'ego
                // ============================================
            else if (obj->objectType == ObjectType::GregoryPatch)
            {
                auto g = std::static_pointer_cast<SceneGregoryPatch>(obj);
                for (auto& wp : g->points)
                {
                    if (auto p = wp.lock())
                    {
                        if (!p->isVirtual) // Wirtualne żyją i umierają z płatem, nie mają globalnego licznika!
                            p->globalSurfacesCount--;
                    }
                }
            }
        }
    }


    // w osobnej pętli usuwamy powiązania z płatami - z uwagi na flage !!!globalCurvesCount == 0!!!
    // dla każdego pending delete - prywatnego i globalnego
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
                    //p->belongsToPatch = false;

                    //tymvzasowo to połaczmy, ale zmieimy na jeden guzik
                    if (guiManager.surfaceDeletionMode == 1 || guiManager.surfaceDeletionMode == 2)
                    {
                        // Tryb 2: usuń punkt tylko jeśli nie jest w żadnej krzywej lub powierzchni
                        if (p->globalCurvesCount == 0 && p->globalSurfacesCount == 0)
                            p->pendingDelete = true;
                    }
                    // Tryb 0: usuń tylko płat (punkty zostają)
                }
            }
        }
        else if (obj->pendingDelete &&
            obj->objectType == ObjectType::GregoryPatch)
        {
            auto s = std::static_pointer_cast<SceneGregoryPatch>(obj);
            for (auto& wp : s->points)
            {
                if (auto p = wp.lock())
                {
                    //p->belongsToPatch = false;

                    //tymvzasowo to połaczmy, ale zmieimy na jeden guzik
                    if (guiManager.surfaceDeletionMode == 1 || guiManager.surfaceDeletionMode == 2)
                    {
                        // Tryb 2: usuń punkt tylko jeśli nie jest w żadnej krzywej lub powierzchni
                        if (p->globalCurvesCount == 0 && p->globalSurfacesCount == 0)
                            p->pendingDelete = true;
                    }
                    // Tryb 0: usuń tylko płat (punkty zostają)
                }
            }
        }
    }

    // wreszczie oznaczenie pynktów do usuniecia
    // tylko tryb globalny
    if (guiManager.deleteSelectedPressed)
    {
        for (auto& obj : sceneObjects)
        {
            if (obj->objectType == ObjectType::Point && obj->isSelected)
            {
                auto p = std::static_pointer_cast<ScenePoint>(obj);
                if (p->globalSurfacesCount == 0
                        //!p->belongsToPatch
                    //&& p->globalCurvesCount == 0  // -> to jakbysmy chcieci aby NIE USUWALY sie nalezace do krzywych punkty
                    )
                {
                    p->pendingDelete = true;
                }
            }
        }
    }

    // Skasuj obiekty (w tym puste krzywe)
    sceneObjects.erase(std::remove_if(sceneObjects.begin(), sceneObjects.end(),
                                      [](const std::shared_ptr<SceneObject>& o) { return o->pendingDelete; }), sceneObjects.end());

}
