#pragma once

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "imgui.h"
#include "sceneTorus.h"
#include "guiManager.h"
#include "bakeTransform.h"
#include "sceneBezierC2.h"
#include "sceneSplineInterpolating.h"
#include "sceneSurface.h"
#include "sceneSerializer.h"


void GuiManager::clearGuiState()
{
    guiDeltaPos[0] = guiDeltaPos[1] = guiDeltaPos[2] = 0.0f;
    guiDeltaScale = 1.0f;
    guiRotAxis[0] = 0.0f; guiRotAxis[1] = 1.0f; guiRotAxis[2] = 0.0f;
    guiRotAngle = 0.0f;
    guiRotQuat[0] = 1.0f; guiRotQuat[1] = 0.0f; guiRotQuat[2] = 0.0f; guiRotQuat[3] = 0.0f;
}

void GuiManager::Draw(std::vector<std::shared_ptr<SceneObject>>& sceneObjects,
                      Cursor& cursor, Camera& camera,
                      AppState& state, // <---- WSZYSTKIE 3 ENUMY ZASTĄPIONE TYM!
                      bool isBoxSelecting, double boxStartX, double boxStartY, double boxEndX, double boxEndY,
                      bool& magicMode, std::shared_ptr<SceneBezier>& magicCurve,
                      bool isCamDragging, Vect3& centerOfSelection)

{
    ImGui::Begin("Sterowanie i Obiekty");

    // =========================================================================
    // PANEL: ZAPIS I ODCZYT SCENY (JSON)
    // =========================================================================
    if (ImGui::CollapsingHeader("Zapis i Odczyt Sceny"))
    {
        // Statyczny bufor tekstowy, dzięki czemu użytkownik może wpisać własną nazwę pliku
        // Domyślnie ustawiamy na "example_scene.json", czyli nazwę z Twoich materiałów
        static char sceneFilename[128] = "example_scene.json";
        ImGui::InputText("Nazwa pliku", sceneFilename, IM_ARRAYSIZE(sceneFilename));

        // Przycisk Zapisu (zajmuje połowę dostępnej szerokości okna)
        if (ImGui::Button("Zapisz Scenę", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0)))
        {
            SceneSerializer::SaveScene(sceneFilename, sceneObjects);
        }
        ImGui::SameLine();

        // Przycisk Odczytu (zajmuje pozostałą przestrzeń)
        if (ImGui::Button("Wczytaj Scenę", ImVec2(-1, 0)))
        {
            // KROK KRYTYCZNY: Czyścimy obecny wektor obiektów sceny,
            // aby wczytywane elementy nie nałożyły się ani nie zdublowały z obecnymi!
            sceneObjects.clear();

            // Czyścimy również naszą nową pulę obiektów podglądu (preview),
            // na wypadek gdyby użytkownik miał otwarty panel tworzenia płata
            if (previewSurface)
            {
                previewSurface = nullptr;
                previewPoints.clear();
            }

            // Wywołujemy nasz loader, który automatycznie odtworzy relacje shared/weak_ptr
            // oraz przełoży układ współrzędnych z OpenGL (Y-up) na Twój świat (Z-up)
            SceneSerializer::LoadScene(sceneFilename, sceneObjects);
        }
    }
    ImGui::Separator();
    // =========================================================================

    if (isBoxSelecting)
    {
        ImGui::GetForegroundDrawList()->AddRect(
                ImVec2((float)boxStartX, (float)boxStartY), ImVec2((float)boxEndX, (float)boxEndY),
                IM_COL32(255, 255, 255, 255), 0.0f, 0, 1.5f
        );
    }

    ImGui::Text("Kursor");
    ImGui::DragFloat3("Pozycja (Scena)", &cursor.transform.posX, 0.1f, min_pos, max_pos);
    //ImGui::Text("Pozycja (Ekran): X: %.1f, Y: %.1f", cursor.screenX, cursor.screenY);
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Ustawienia Stereoskopii (3D Anaglif)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Wlacz Tryb Stereoskopowy", &isStereoMode);
        if (isStereoMode)
        {
            ImGui::Indent();
            ImGui::SliderFloat("Rozstaw Oczu (IOD)", &eyeSeparation, 0.0f, 5.0f);
            ImGui::SliderFloat("Odleglosc Ekranu (ZPD)", &focalDistance, 1.0f, 100.0f);
            ImGui::Unindent();
        }
    }
    ImGui::Separator();

    if (ImGui::Button("Dodaj Torus"))
    {
        auto t = std::make_shared<SceneTorus>("Torus " + std::to_string(sceneObjects.size()+1), cursor.transform);
        t->Init();
        sceneObjects.push_back(t);
    }
    ImGui::SameLine();
    if (ImGui::Button("Dodaj Punkt"))
    {
        auto p = std::make_shared<ScenePoint>("Punkt " + std::to_string(sceneObjects.size()+1), cursor.transform);
        p->Init();
        sceneObjects.push_back(p);
        for(auto& obj : sceneObjects)
        {
            // DZIAŁA DLA 3 KRZYWYCH
            if (obj->objectType == ObjectType::BezierCurveC0 ||
            obj->objectType == ObjectType::BezierCurveC2 ||
            obj->objectType == ObjectType::SplineInterpolating)
            {
                auto b = std::static_pointer_cast<SceneBezier>(obj);
                if (b->isSelected)
                {
                    b->points.push_back(p);
                    p->globalCurvesCount++;
                }
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Usun Zaznacz."))
    {
        deleteSelectedPressed = true;
    }
    ImGui::Separator();

    bool isGuiDisabledThisFrame = false;
    static int selectedBezierIndex = 0;

    if (!magicMode)
    {
        std::vector<std::string> bezierNames;
        std::vector<std::shared_ptr<SceneBezier>> bezierPointers;
        bezierNames.push_back("Nowy Bezier");

        for (auto& obj : sceneObjects)
        {
            // ŁAPIEMY trzy KRZYWE DO COMBOBOXA
            if (obj->objectType == ObjectType::BezierCurveC0 ||
            obj->objectType == ObjectType::BezierCurveC2 ||
            obj->objectType == ObjectType::SplineInterpolating)
            {
                auto b = std::static_pointer_cast<SceneBezier>(obj);
                bezierNames.push_back(b->name);
                bezierPointers.push_back(b);
            }
        }

        if (selectedBezierIndex >= bezierNames.size())
            selectedBezierIndex = 0;

        ImGui::SetNextItemWidth(120);
        if (ImGui::BeginCombo("##bezierCombo", bezierNames[selectedBezierIndex].c_str()))
        {
            for (int i = 0; i < bezierNames.size(); i++)
            {
                bool is_selected = (selectedBezierIndex == i);
                if (ImGui::Selectable(bezierNames[i].c_str(), is_selected))
                    selectedBezierIndex = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();

        // Zamiast jednego przycisku z zagnieżdżeniami, od razu wyświetlamy właściwe opcje!
        if (selectedBezierIndex == 0)
        {
            if (ImGui::Button("Stworz Nowa C0"))
            {
                std::vector<std::shared_ptr<ScenePoint>> selPts;
                for(auto& obj : sceneObjects)
                {
                    if (obj->objectType == ObjectType::Point)
                    {
                        auto p = std::static_pointer_cast<ScenePoint>(obj);
                        if (p->isSelected)
                            selPts.push_back(p);
                    }
                }
                if (!selPts.empty())
                {
                    auto b = std::make_shared<SceneBezierC0>("Bezier C0 " + std::to_string(sceneObjects.size()+1), Transformations());
                    b->Init();
                    for(auto& p : selPts)
                    {
                        b->points.push_back(p);
                        p->globalCurvesCount++;
                    }
                    sceneObjects.push_back(b);
                    magicMode = true;
                    magicCurve = b;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Stworz Nowa C2"))
            {
                std::vector<std::shared_ptr<ScenePoint>> selPts;
                for(auto& obj : sceneObjects)
                {
                    if (obj->objectType == ObjectType::Point)
                    {
                        auto p = std::static_pointer_cast<ScenePoint>(obj);
                        if (p->isSelected)
                            selPts.push_back(p);
                    }
                }
                if (!selPts.empty())
                {
                    auto b2 = std::make_shared<SceneBezierC2>("Bezier C2 " + std::to_string(sceneObjects.size() + 1),
                                                              Transformations());
                    b2->Init();
                    for (auto &p: selPts) {
                        b2->points.push_back(p);
                        p->globalCurvesCount++;
                    }
                    sceneObjects.push_back(b2);
                    magicMode = true;
                    magicCurve = b2;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Stworz Interpolujaca C2"))
            {
                std::vector<std::shared_ptr<ScenePoint>> selPts;
                for(auto& obj : sceneObjects)
                {
                    if (obj->objectType == ObjectType::Point)
                    {
                        auto p = std::static_pointer_cast<ScenePoint>(obj);
                        if (p->isSelected)
                            selPts.push_back(p);
                    }
                }
                if (!selPts.empty()) {
                    auto s = std::make_shared<SceneSplineInterpolating>("Splajn " + std::to_string(sceneObjects.size() + 1), Transformations());
                    s->Init();
                    for (auto &p: selPts)
                    {
                        s->points.push_back(p);
                        p->globalCurvesCount++;
                    }
                    sceneObjects.push_back(s);
                    magicMode = true;
                    magicCurve = s; // Działa idealnie dzięki dziedziczeniu po SceneBezier!
                }
            }
        }
        else
        {
            if (ImGui::Button("Dodaj Punkty do Krzywej"))
            {
                std::vector<std::shared_ptr<ScenePoint>> selPts;
                for(auto& obj : sceneObjects)
                {
                    if (obj->objectType == ObjectType::Point)
                    {
                        auto p = std::static_pointer_cast<ScenePoint>(obj);
                        if (p->isSelected)
                            selPts.push_back(p);
                    }
                }
                auto b = bezierPointers[selectedBezierIndex - 1];
                for(auto& p : selPts)
                {
                    bool exists = false; // czu juz ten punkt jest w tej krzywej
                    for(auto& wp : b->points)
                    {
                        if (wp.lock() == p)
                        {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists)
                    {
                        b->points.push_back(p);
                        p->globalCurvesCount++;
                    }
                }
                magicMode = true;
                magicCurve = b;
            }
        }

        ImGui::Separator();


        if (forceClosePanel)
        {
            ImGui::SetNextItemOpen(false);
            forceClosePanel = false;
        }

        isNewSurfacePanelOpen = ImGui::CollapsingHeader("Nowy Plat Laczony");

        if (isNewSurfacePanelOpen)
        {
            if (previewSurface == nullptr)
                refreshPreview(cursor);

            bool changed = false;

            // minimalna ilosc platów
            int minPatchesU = 1; // płaszczyzna 1
            if (newSurfIsCylinder)
            {
                minPatchesU = (newSurfType == 1) ? 3 : 2; // Walec C2: 3, Walec C0: 2
            }

            if (newSurfPatchesU < minPatchesU)
            {
                newSurfPatchesU = minPatchesU;
                changed = true; // Wymusza przeliczenie
            }



            if (ImGui::SliderInt("Platy U", &newSurfPatchesU, minPatchesU, 10)) changed = true;
            if (ImGui::SliderInt("Platy V", &newSurfPatchesV, 1, 10)) changed = true;
            if (ImGui::DragFloat("Szer/Promien", &newSurfDimU, 0.1f, 0.1f, 50.0f)) changed = true;
            if (ImGui::DragFloat("Wysokosc", &newSurfDimV, 0.1f, 0.1f, 50.0f)) changed = true;
            if (ImGui::Combo("Typ", &newSurfType, "Bezier C0\0B-Spline C2\0")) changed = true;
            if (ImGui::Checkbox("Walec", &newSurfIsCylinder)) changed = true;

            if (changed)
                refreshPreview(cursor);

            if (ImGui::Button("Stworz Plat", ImVec2(-1, 30)))
            {
                for (auto& p : previewPoints)
                {
                    // Dodajemy tylko unikalne (ważne przy walcu!)
                    if (std::find(sceneObjects.begin(), sceneObjects.end(), p) == sceneObjects.end())
                    {
                        p->belongsToPatch = true;
                        p->color[0] = 1.0f;
                        p->color[1] = 1.0f;
                        p->color[2] = 0.0f; // Reset do żółtego
                        sceneObjects.push_back(p);
                    }
                }
                previewSurface->color[0] = 1.0f;
                previewSurface->color[1] = 1.0f;
                previewSurface->color[2] = 0.0f;
                previewSurface->name = (newSurfType == 0 ? "Plat C0" : "Plat C2");
                sceneObjects.push_back(previewSurface);

                previewSurface = nullptr;
                previewPoints.clear();
                forceClosePanel = true; // Zamykamy panel
            }
        }
        else
        {
            if (previewSurface)
            {
                previewSurface = nullptr;
                previewPoints.clear();
            }
        }



    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Zapisz zmiany (Krzywa)"))
        {
            magicMode = false;
            magicCurve = nullptr;
        }
        ImGui::PopStyleColor();
        ImGui::BeginDisabled();
        isGuiDisabledThisFrame = true;
    }

    ImGui::Separator();

    centerOfSelection = Vect3(0,0,0);
    Vect3 globalCenter(0,0,0);
    int selCount = 0;
    int globalCount = 0;

    for (auto& obj : sceneObjects)
    {
        //do środka ciezkosci tych obiektów nie tzreba
        if (obj->objectType == ObjectType::BezierCurveC0 ||
            obj->objectType == ObjectType::BezierCurveC2 ||
            obj->objectType == ObjectType::SplineInterpolating ||
            obj->objectType == ObjectType::BezierSurfaceC0 ||
            obj->objectType == ObjectType::BezierCurveC2)
            continue;


        globalCenter += obj->transformations.getPosition();
        globalCount++;

        bool isPartOfSelection = false;
        if (obj->objectType == ObjectType::Point)
        {
            auto p = std::static_pointer_cast<ScenePoint>(obj);
            if (p->isAnyWaySelected())
                isPartOfSelection = true;
        }
        else
        {
            if (obj->isSelected)
                isPartOfSelection = true;
        }

        if (isPartOfSelection)
        {
            centerOfSelection += obj->transformations.getPosition();
            selCount++;
        }
    }

    if (selCount > 0)
        centerOfSelection = Vect3(centerOfSelection.x/(float)selCount, centerOfSelection.y/(float)selCount, centerOfSelection.z/(float)selCount);

    if (globalCount > 0)
        globalCenter = Vect3(globalCenter.x/(float)globalCount, globalCenter.y/(float)globalCount, globalCenter.z/(float)globalCount);

    ImGui::Text("Zaznaczonych: %d | Srodek: %.1f, %.1f, %.1f", selCount, centerOfSelection.x, centerOfSelection.y, centerOfSelection.z);

    if (ImGui::Button("Wysrodkuj obiekty w (0,0,0)"))
    {
        clearGuiState();
        bool hasSelection = (selCount > 0);
        Vect3 offset = hasSelection ? centerOfSelection : globalCenter;
        for (auto& obj : sceneObjects)
        {
            bool isSelected = false;
            if(obj->objectType == ObjectType::Point)
            {
                auto p = std::static_pointer_cast<ScenePoint>(obj);
                isSelected = p->isAnyWaySelected();

                if(!hasSelection || isSelected)
                    p->wasGuiEdited = true;

            }
            else
                isSelected = obj->isSelected;

            if (!hasSelection || isSelected)
            {
                obj->transformations.posX -= offset.x;
                obj->transformations.posY -= offset.y;
                obj->transformations.posZ -= offset.z;
            }
        }


    }

    ImGui::RadioButton("Zaznacz. Lokalne", reinterpret_cast<int *>(&state.transformMode), LOCAL);
    ImGui::SameLine(); ImGui::RadioButton("Wspolny Srodek", reinterpret_cast<int *>(&state.transformMode), COMMON_CENTER);
    ImGui::SameLine(); ImGui::RadioButton("Wzgledem Kursora", reinterpret_cast<int *>(&state.transformMode), CURSOR_CENTER);

    ImGui::Separator();
    ImGui::Text("Metoda transformacji:");

    if (ImGui::RadioButton("Mysz (Skroty klawiszowe)", reinterpret_cast<int *>(&state.inputMode), INPUT_MOUSE) && state.prevInputMode == INPUT_GUI)
    {
        Vect3 center =  ((state.transformMode == CURSOR_CENTER) ? cursor.transform.getPosition() : centerOfSelection);

        // bardzo ważne że zmuszamy do wypieczenia zmian w tym miejscu
        // oraz bardzo ważne że bakeTransformation działa niezaleznie od myszka/GUI
        bakeTransformations(sceneObjects, getGuiDelta(), state.transformMode, center);
        wasBaked = true;

        clearGuiState();
    }
    ImGui::SameLine();
    ImGui::RadioButton("Wartosci z GUI (Live Preview)", reinterpret_cast<int *>(&state.inputMode), INPUT_GUI);

    state.prevInputMode = state.inputMode;

    if (state.inputMode == INPUT_GUI)
    {
        ImGui::Separator();
        ImGui::DragFloat3("Przesuniecie (XYZ)", guiDeltaPos, 0.1f, min_pos, max_pos);
        if (ImGui::DragFloat("Skala (Mnoznik)", &guiDeltaScale, 0.05f, min_scale, max_scale))
            guiDeltaScale = std::max(0.01f, guiDeltaScale);

        ImGui::Text("Tryb Rotacji:");
        ImGui::RadioButton("Os i Kat", &guiRotMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Kwaternion", &guiRotMode, 1);

        if (guiRotMode != prevGuiRotMode)
        {
            guiRotAxis[0] = 0.0f;
            guiRotAxis[1] = 1.0f;
            guiRotAxis[2] = 0.0f;
            guiRotAngle = 0.0f;
            guiRotQuat[0] = 1.0f;
            guiRotQuat[1] = 0.0f;
            guiRotQuat[2] = 0.0f;
            guiRotQuat[3] = 0.0f;
            prevGuiRotMode = guiRotMode;
        }

        if (guiRotMode == 0)
        {
            if (ImGui::DragFloat3("Wektor Osi", guiRotAxis, 0.01f, min_axis, max_axis))
            {
                if (std::abs(guiRotAxis[0]) < 0.0001f && std::abs(guiRotAxis[1]) < 0.0001f && std::abs(guiRotAxis[2]) < 0.0001f)
                    guiRotAxis[1] = 1.0f;
            }
            ImGui::DragFloat("Kat (Stopnie)", &guiRotAngle, 1.0f, min_angle, max_angle);
        }
        else if (guiRotMode == 1)
        {
            if (ImGui::DragFloat4("Kwaternion (WXYZ)", guiRotQuat, 0.01f, -1.0f, 1.0f))
            {
                Quaternion tempQ(guiRotQuat[0], guiRotQuat[1], guiRotQuat[2], guiRotQuat[3]);
                tempQ.normalize();
                guiRotQuat[0] = tempQ.w;
                guiRotQuat[1] = tempQ.x;
                guiRotQuat[2] = tempQ.y;
                guiRotQuat[3] = tempQ.z;
            }
        }

        ImGui::Spacing();
        if (ImGui::Button("Zachowaj Zmiany", ImVec2(150, 30)))
        {
            Vect3 center = ((state.transformMode == CURSOR_CENTER) ? cursor.transform.getPosition() : centerOfSelection);


            bakeTransformations(sceneObjects, getGuiDelta(), state.transformMode, center);
            wasBaked = true;

            clearGuiState();
        }
        ImGui::SameLine();
        if (ImGui::Button("Wyczysc", ImVec2(100, 30)))
            clearGuiState();
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Sterowanie Kamera"))
    {
        if (!isCamDragging)
        {
            bool camChanged = false;
            camChanged |= ImGui::DragFloat3("Pozycja Kamery", &camera.position.x, 0.1f);
            camChanged |= ImGui::DragFloat3("Cel Kamery (Target)", &camera.target.x, 0.1f);
            if (ImGui::Button("Reset Kamery"))
            {
                camera.position = Vect3(5.0f, -25.0f, 20.0f);
                camera.target = Vect3(0.0f, 0.0f, 0.0f);
                camChanged = true;
            }
            if (camChanged)
                camera.enforceConstraints();
        }
    }

    ImGui::Separator();
    ImGui::Text("Lista Obiektow na Scenie:");



    if (ImGui::TreeNodeEx("Obiekty", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (auto& obj : sceneObjects)
        {
            if(obj->objectType != ObjectType::Point)
                renderObjectGuiRow(obj, magicMode, magicCurve);
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Punkty", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (auto& obj : sceneObjects)
        {
            if (obj->objectType == ObjectType::Point)
            {
                renderObjectGuiRow(obj, magicMode, magicCurve);
            }
        }
        ImGui::TreePop();
    }

    if (isGuiDisabledThisFrame)
        ImGui::EndDisabled();

    ImGui::End();
}


void GuiManager::renderObjectGuiRow(std::shared_ptr<SceneObject>& obj, bool& magicMode, std::shared_ptr<SceneBezier>& magicCurve)
{
    ImGui::PushID(obj.get());

    if (obj->objectType == ObjectType::Point)
    {
        auto p = std::static_pointer_cast<ScenePoint>(obj);

        ImVec4 textColor = p->isAnyWaySelected() ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f);

        ImGui::TextColored(textColor, "K:%d P:%d ", p->globalCurvesCount, (p->belongsToPatch ? 1 : 0));
        ImGui::SameLine();
    }

    if (ImGui::Checkbox("##sel", &obj->isSelected))
    {
        // Jeśli klikniesz jakikolwiek checkbox na liście, zapalamy flagi dwie
        // w osobnj funkcji poprawiamy liczniki wyselectowanych krzywych/płatów dla punktów
        wasSelectionChanged = true;
        obj->wasGuiSelectionChanged = true;
    }
    ImGui::SameLine();

    char nameBuf[128];
    strcpy(nameBuf, obj->name.c_str());
    ImGui::SetNextItemWidth(150);
    if (ImGui::InputText("##name", nameBuf, IM_ARRAYSIZE(nameBuf)))
        obj->name = nameBuf;

    ImGui::SameLine();

    // Jeśli to punkt i nie moze zostac usuniety (na razie tylko gdy należy do płata), blokujemy całkowicie usunięcie
    if (obj->objectType == ObjectType::Point && !std::static_pointer_cast<ScenePoint>(obj)->canBeDeleted())
    {
        ImGui::BeginDisabled();
        ImGui::Button("Usun (Zablokowane)");
        ImGui::EndDisabled();
    }
    else
    {
        if (ImGui::Button("Usun"))
            obj->pendingDelete = true;
    }

    if (ImGui::TreeNode("Edytuj"))
    {
        ImGui::Indent();
        ImGui::ColorEdit3("Kolor", obj->color);


        if(obj->objectType != ObjectType::BezierCurveC0 &&
           obj->objectType != ObjectType::BezierCurveC2 &&
           obj->objectType != ObjectType::SplineInterpolating &&
           obj->objectType != ObjectType::BezierSurfaceC0 &&
           obj->objectType != ObjectType::BezierSurfaceC2)
        {
            ImGui::Text("Transformacja obiektu:");
            if (ImGui::DragFloat3("Pozycja (XYZ)", &obj->transformations.posX, 0.1f, min_pos, max_pos))
            {
                if (obj->objectType == ObjectType::Point)
                {
                    auto p = std::static_pointer_cast<ScenePoint>(obj);
                    p->wasGuiEdited = true;
                }
            }
            ImGui::DragFloat("Skala", &obj->transformations.scale, 0.05f, min_scale, max_scale);

            float quatVals[4] =
                    {obj->transformations.rotation.w,
                     obj->transformations.rotation.x,
                     obj->transformations.rotation.y,
                     obj->transformations.rotation.z
                    };

            if (ImGui::DragFloat4("Rotacja (WXYZ)", quatVals, 0.01f, -1.0f, 1.0f))
            {
                obj->transformations.rotation.w = quatVals[0];
                obj->transformations.rotation.x = quatVals[1];
                obj->transformations.rotation.y = quatVals[2];
                obj->transformations.rotation.z = quatVals[3];
                obj->transformations.rotation.normalize();
            }
        }

        if (obj->objectType == ObjectType::BezierSurfaceC0 || obj->objectType == ObjectType::BezierSurfaceC2)
        {
            auto s = std::static_pointer_cast<SceneSurface>(obj);
            ImGui::Text("Gestosc siatki (samples):");
            ImGui::SliderInt("U", &s->samplesU, 1, 64);
            ImGui::SliderInt("V", &s->samplesV, 1, 64);
            ImGui::Checkbox("Pokaz wielobok kontrolny", &s->showPolygon);

            ImGui::Separator();
            ImGui::Text("Tryb usuwania:");
            ImGui::RadioButton("Tylko plat", &surfaceDeletionMode, 0);
            ImGui::RadioButton("Plat i wszystkie punkty", &surfaceDeletionMode, 1);
            ImGui::RadioButton("Smart (tylko nieuzywane pkt)", &surfaceDeletionMode, 2);
        }
        else if (obj->objectType == ObjectType::BezierCurveC0)
        {
            auto b = std::static_pointer_cast<SceneBezierC0>(obj);

            ImGui::Checkbox("Pokaz lamana", &b->showPolygon);
            if (ImGui::Button("Dodaj calkowicie nowe punkty"))
            {
                magicMode = true;
                magicCurve = b;
            }

            ImGui::Text("Punkty kontrolne:");
            for (auto it = b->points.begin(); it != b->points.end(); )
            {
                if (auto ptr = it->lock())
                {
                    ImGui::Text(" - %s", ptr->name.c_str());
                    ImGui::SameLine();
                    ImGui::PushID(ptr.get());

                    if (ImGui::Button("Usun z krzywej"))
                    {
                        it = b->points.erase(it);
                        ptr->globalCurvesCount--;
                    }
                    else ++it;

                    ImGui::PopID();
                }
                else
                {
                    it = b->points.erase(it);
                }
            }
        }
        else if (obj->objectType == ObjectType::BezierCurveC2)
        {
            auto b2 = std::static_pointer_cast<SceneBezierC2>(obj);

            ImGui::Checkbox("Pokaz lamana", &b2->showPolygon);

            if (ImGui::Button("Dodaj calkowicie nowe punkty"))
            {
                magicMode = true;
                magicCurve = b2;
            }



            ImGui::Text("Baza wielomianowa:");
            int basis = (int)b2->currentBasis;
            if (ImGui::RadioButton("B-Spline", &basis, 0))
                b2->currentBasis = BezierBasisMode::B_SPLINE;
            ImGui::SameLine();
            if (ImGui::RadioButton("Bernstein", &basis, 1))
                b2->currentBasis = BezierBasisMode::BERNSTEIN;

            ImGui::Text("Punkty (de Boora):");
            for (auto it = b2->points.begin(); it != b2->points.end(); )
            {
                if (auto ptr = it->lock())
                {
                    ImGui::Text(" - %s", ptr->name.c_str());
                    ImGui::SameLine();
                    ImGui::PushID(ptr.get());

                    if (ImGui::Button("Usun z krzywej"))
                    {
                        it = b2->points.erase(it);
                        ptr->globalCurvesCount--;
                    }
                    else ++it;

                    ImGui::PopID();
                }
                else
                {
                    it = b2->points.erase(it);
                }
            }
        }
        else if (obj->objectType == ObjectType::SplineInterpolating)
        {
            auto s = std::static_pointer_cast<SceneSplineInterpolating>(obj);

            ImGui::Checkbox("Pokaz lamana", &s->showPolygon);

            if (ImGui::Button("Dodaj calkowicie nowe punkty"))
            {
                magicMode = true;
                magicCurve = s;
            }

            ImGui::Text("Tryb rysowania/baza:");
            int basis = (int)s->currentBasis;
            if (ImGui::RadioButton("Algebraiczna (GPU Shader)", &basis, 0))
                s->currentBasis = InterpolationBasisMode::ALGEBRAIC;
            ImGui::SameLine();
            if (ImGui::RadioButton("Bernstein (Wirtualne)", &basis, 1))
                s->currentBasis = InterpolationBasisMode::BERNSTEIN;

            ImGui::Text("Punkty Interpolacyjne:");
            for (auto it = s->points.begin(); it != s->points.end(); )
            {
                if (auto ptr = it->lock())
                {
                    ImGui::Text(" - %s", ptr->name.c_str());
                    ImGui::SameLine();
                    ImGui::PushID(ptr.get());

                    if (ImGui::Button("Usun z krzywej"))
                    {
                        it = s->points.erase(it);
                        ptr->globalCurvesCount--;
                    }
                    else ++it;

                    ImGui::PopID();
                }
                else
                {
                    it = s->points.erase(it);
                }
            }
        }
        else if(obj->objectType == ObjectType::Torus)
        {
            auto t = std::static_pointer_cast<SceneTorus>(obj);
            ImGui::Text("Geometria Torusa:");
            bool needsUpdate = false;
            needsUpdate |= ImGui::SliderFloat("R", &t->R, min_R, max_R);
            needsUpdate |= ImGui::SliderFloat("r", &t->r, min_r, max_r);
            needsUpdate |= ImGui::SliderInt("density R", &t->density_R, min_density_R, max_density_R);
            needsUpdate |= ImGui::SliderInt("density r", &t->density_r, min_density_r, max_density_r);
            if (needsUpdate)
                t->UpdateBuffers();
        }
        else if (obj->objectType == ObjectType::Point)
        {
            auto p = std::static_pointer_cast<ScenePoint>(obj);
            ImGui::Text("Geometria Punktu:");
            ImGui::SliderFloat("Rozmiar", &p->size, 1.0f, 6.0f);
        }
        ImGui::Unindent();
        ImGui::Separator();
        ImGui::TreePop();
    }
    ImGui::PopID();
}


void GuiManager::refreshPreview(const Cursor& cursor)
{
    bool isC0 = (newSurfType == 0);

    int minPatchesU = 1; // płaszczyzna: 1
    if (newSurfIsCylinder)
    {
        minPatchesU = isC0 ? 2 : 3; // Walec C2: 3, Walec C0: 2
    }

    if (newSurfPatchesU < minPatchesU)
        newSurfPatchesU = minPatchesU;
    if (newSurfPatchesV < 1)
        newSurfPatchesV = 1;


    int uniqueU = 0;
    int sizeU = 0;

    if (newSurfIsCylinder)
    {
        if (isC0)
        {
            uniqueU = newSurfPatchesU * 3;
        }
        else
        {
            uniqueU = newSurfPatchesU; // Suwak już wymusza minimum 3
        }
        sizeU = uniqueU + 1; // 1 powtorzony na końcu dla formatu
    }
    else
    {
        uniqueU = isC0 ? (newSurfPatchesU * 3 + 1) : (newSurfPatchesU + 3);
        sizeU = uniqueU;
    }

    int sizeV = isC0 ? (newSurfPatchesV * 3 + 1) : (newSurfPatchesV + 3);
    int requiredUniquePoints = uniqueU * sizeV;


    while (previewPoints.size() < requiredUniquePoints)
    {
        auto p = std::make_shared<ScenePoint>("P", Transformations());
        p->Init();
        p->color[0] = 0.5f;
        p->color[1] = 0.5f;
        p->color[2] = 0.5f;
        previewPoints.push_back(p);
    }
    if (previewPoints.size() > requiredUniquePoints)
    {
        previewPoints.resize(requiredUniquePoints);
    }


    ObjectType requiredSurfaceType = isC0 ? ObjectType::BezierSurfaceC0 : ObjectType::BezierSurfaceC2;

    if (previewSurface == nullptr || previewSurface->objectType != requiredSurfaceType)
    {
        if (isC0)
            previewSurface = std::make_shared<SceneSurfaceC0>("Preview Surface", Transformations());
        else
            previewSurface = std::make_shared<SceneSurfaceC2>("Preview Surface", Transformations());
    }

    previewSurface->sizeU = sizeU;
    previewSurface->sizeV = sizeV;
    previewSurface->isCylinder = newSurfIsCylinder;
    previewSurface->color[0] = 0.6f;
    previewSurface->color[1] = 0.6f;
    previewSurface->color[2] = 0.6f;
    previewSurface->points.clear();


    Vect3 center = cursor.transform.getPosition();
    int uniqueIdx = 0;

    for (int v = 0; v < sizeV; ++v)
    {
        for (int u = 0; u < sizeU; ++u)
        {
            // Zgodnie z formatem: powtórzenie na końcu rzędu w walcu
            if (newSurfIsCylinder && u == uniqueU)
            {
                previewSurface->points.push_back(previewSurface->points[v * sizeU]);
                continue;
            }

            auto p = previewPoints[uniqueIdx++];
            Vect3 localPos;

            if (newSurfIsCylinder)
            {
                float delta = 2.0f * (float)M_PI / (float)uniqueU;
                float H = newSurfDimV;

                if (isC0)
                {
                    float R = newSurfDimU;
                    // kąt przypadający na jeden plat
                    float theta = 2.0f * (float)M_PI / (float)newSurfPatchesU;

                    // apriksymacja dla beziera
                    float L = (4.0f / 3.0f) * tan(theta / 4.0f) * R;


                    int patchIdx = u / 3;
                    int pointType = u % 3;

                    float angle = (float)patchIdx * theta;
                    float nextAngle = (float)(patchIdx + 1) * theta;

                    float x, z;
                    if (pointType == 0)
                    {
                        // P0: Węzeł leży idealnie na okręgu
                        x = R * cos(angle);
                        z = R * sin(angle);
                    }
                    else if (pointType == 1)
                    {
                        // P1: od P0 wzdluz stycznej
                        x = R * cos(angle) - L * sin(angle);
                        z = R * sin(angle) + L * cos(angle);
                    }
                    else if (pointType == 2)
                    {
                        // P2: Cofamy od P3, po wektorze stycznym
                        x = R * cos(nextAngle) + L * sin(nextAngle);
                        z = R * sin(nextAngle) - L * cos(nextAngle);
                    }

                    float y = (float)v * (H / (newSurfPatchesV * 3.0f));
                    localPos = Vect3(x, y, z);
                }
                else // C2 (B-Spline)
                {
                    float R_dB = newSurfDimU * 3.0f / (cos(delta) + 2.0f);
                    float angle = (float)u * delta;
                    float y = (float)(v - 1) * (H / newSurfPatchesV);
                    localPos = Vect3(R_dB * cos(angle), y, R_dB * sin(angle));
                }
            }
            else
            {
                // PŁASZCZYZNA
                if (isC0)
                {
                    localPos = Vect3((float)u * (newSurfDimU / 3.0f), 0.0f, (float)v * (newSurfDimV / 3.0f));
                }
                else
                { // C2
                    localPos = Vect3((float)(u - 1) * newSurfDimU, 0.0f, (float)(v - 1) * newSurfDimV);
                }
            }

            p->transformations.setPosition(center + localPos);
            previewSurface->points.push_back(p);
        }
    }

    // Przebudowa indeksów
    previewSurface->Init();
}
