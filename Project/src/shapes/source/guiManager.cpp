#pragma once

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "imgui.h"
#include "sceneTorus.h"
#include "guiManager.h"
#include "bakeTransform.h"


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
                      TransformManager& tm, // <---- WSZYSTKIE 3 ENUMY ZASTĄPIONE TYM!
                      bool isBoxSelecting, double boxStartX, double boxStartY, double boxEndX, double boxEndY,
                      bool& magicMode, std::shared_ptr<SceneBezierC0>& magicCurve,
                      bool isCamDragging, Vect3& centerOfSelection)

{
    ImGui::Begin("Sterowanie i Obiekty");

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
            if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj))
            {
                if (b->isSelected)
                    b->points.push_back(p);
            }
        }
    }
    ImGui::SameLine();

    bool isGuiDisabledThisFrame = false;
    static int selectedBezierIndex = 0;

    if (!magicMode)
    {
        std::vector<std::string> bezierNames;
        std::vector<std::shared_ptr<SceneBezierC0>> bezierPointers;
        bezierNames.push_back("Nowy Bezier");

        for (auto& obj : sceneObjects)
        {
            if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj))
            {
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
                {
                    selectedBezierIndex = i;
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        if (ImGui::Button("Stworz/Dodaj"))
        {
            std::vector<std::shared_ptr<ScenePoint>> selPts;
            for(auto& obj : sceneObjects)
            {
                if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj))
                {
                    if (p->isSelected)
                        selPts.push_back(p);
                }
            }
            if (selectedBezierIndex == 0)
            {
                if (!selPts.empty())
                {
                    auto b = std::make_shared<SceneBezierC0>("Bezier " + std::to_string(sceneObjects.size()+1), Transformations());
                    b->Init();
                    for(auto& p : selPts)
                        b->points.push_back(p);

                    sceneObjects.push_back(b);
                    magicMode = true;
                    magicCurve = b;
                }
            }
            else
            {
                auto b = bezierPointers[selectedBezierIndex - 1];
                for(auto& p : selPts)
                {
                    bool exists = false;
                    for(auto& wp : b->points)
                    {
                        if (wp.lock() == p)
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists)
                        b->points.push_back(p);
                }
                magicMode = true;
                magicCurve = b;
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

    ImGui::SameLine();
    if (ImGui::Button("Usun Zaznacz."))
    {
        sceneObjects.erase(std::remove_if(sceneObjects.begin(), sceneObjects.end(),
                                          [](const std::shared_ptr<SceneObject>& o) { return o->isSelected; }), sceneObjects.end());
    }
    ImGui::Separator();

    centerOfSelection = Vect3(0,0,0);
    Vect3 globalCenter(0,0,0);
    int selCount = 0;
    int globalCount = 0;

    for (auto& obj : sceneObjects)
    {
        if (obj->objectType == ObjectType::BezierCurveC0)
            continue;


        globalCenter += obj->transformations.getPosition();
        globalCount++;

        bool isPartOfSelection = false;
        if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj))
        {
            if (p->isSelected || p->selectedCurvesCount > 0)
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
            if (!hasSelection || obj->isSelected)
            {
                obj->transformations.posX -= offset.x;
                obj->transformations.posY -= offset.y;
                obj->transformations.posZ -= offset.z;
            }
        }
    }

    ImGui::RadioButton("Zaznacz. Lokalne", reinterpret_cast<int *>(&tm.transformMode), LOCAL);
    ImGui::SameLine(); ImGui::RadioButton("Wspolny Środek", reinterpret_cast<int *>(&tm.transformMode), COMMON_CENTER);
    ImGui::SameLine(); ImGui::RadioButton("Wzgledem Kursora", reinterpret_cast<int *>(&tm.transformMode), CURSOR_CENTER);
    ImGui::SameLine(); ImGui::RadioButton("Cala Scena", reinterpret_cast<int *>(&tm.transformMode), ENTIRE_SCENE);

    ImGui::Separator();
    ImGui::Text("Metoda transformacji:");

    if (ImGui::RadioButton("Mysz (Skróty klawiszowe)", reinterpret_cast<int *>(&tm.inputMode), INPUT_MOUSE) && tm.prevInputMode == INPUT_GUI)
    {
        clearGuiState();
    }
    ImGui::SameLine();
    ImGui::RadioButton("Wartosci z GUI (Live Preview)", reinterpret_cast<int *>(&tm.inputMode), INPUT_GUI);

    tm.prevInputMode = tm.inputMode;

    if (tm.inputMode == INPUT_GUI)
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
            Quaternion previewQuat(1.0f, 0.0f, 0.0f, 0.0f);
            if (guiRotMode == 0)
                previewQuat = Quaternion::fromAxisAngle(guiRotAxis[0], guiRotAxis[1], guiRotAxis[2], guiRotAngle * (float)M_PI / 180.0f);
            else if (guiRotMode == 1)
                previewQuat = Quaternion(guiRotQuat[0], guiRotQuat[1], guiRotQuat[2], guiRotQuat[3]);
            previewQuat.normalize();

            if (tm.transformMode == LOCAL)
            {
                for (auto& obj : sceneObjects)
                {
                    if (std::dynamic_pointer_cast<SceneBezierC0>(obj))
                        continue;

                    bool shouldBake = obj->isSelected;
                    if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj))
                    {
                        if (p->selectedCurvesCount > 0)
                            shouldBake = true;
                    }

                    if (shouldBake)
                    {
                        obj->transformations.posX += guiDeltaPos[0];
                        obj->transformations.posY += guiDeltaPos[1];
                        obj->transformations.posZ += guiDeltaPos[2];

                        obj->transformations.scale = std::max(0.01f, obj->transformations.scale * guiDeltaScale);

                        obj->transformations.rotation = previewQuat * obj->transformations.rotation;
                        obj->transformations.rotation.normalize();
                    }
                }
            }
            else
            {
                Vect3 center = (tm.transformMode == ENTIRE_SCENE) ?
                               Vect3(0,0,0) :
                               ((tm.transformMode == CURSOR_CENTER) ? cursor.transform.getPosition() : centerOfSelection);

                Transformations tempGroup;
                tempGroup.posX = guiDeltaPos[0];
                tempGroup.posY = guiDeltaPos[1];
                tempGroup.posZ = guiDeltaPos[2];
                tempGroup.scale = guiDeltaScale;
                tempGroup.rotation = previewQuat;
                bakeGroupTransform(sceneObjects, tempGroup, center, tm.transformMode == ENTIRE_SCENE);
            }
            clearGuiState();
        }
        ImGui::SameLine();
        if (ImGui::Button("Wyczysc", ImVec2(100, 30)))
            clearGuiState();
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Sterowanie Kamerą"))
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
            if (!std::dynamic_pointer_cast<ScenePoint>(obj))
                renderObjectGuiRow(obj, magicMode, magicCurve);
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Punkty", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (auto& obj : sceneObjects)
        {
            if (std::dynamic_pointer_cast<ScenePoint>(obj))
                renderObjectGuiRow(obj, magicMode, magicCurve);
        }
        ImGui::TreePop();
    }

    if (isGuiDisabledThisFrame) ImGui::EndDisabled();
    ImGui::End();
}


void GuiManager::renderObjectGuiRow(std::shared_ptr<SceneObject>& obj, bool& magicMode, std::shared_ptr<SceneBezierC0>& magicCurve) const
{
    ImGui::PushID(obj.get());

    if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj))
    {
        if (p->selectedCurvesCount > 0)
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "%d ", p->selectedCurvesCount);
        else
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "0 ");

        ImGui::SameLine();
    }

    ImGui::Checkbox("##sel", &obj->isSelected); ImGui::SameLine();

    char nameBuf[128]; strcpy(nameBuf, obj->name.c_str());
    ImGui::SetNextItemWidth(150);
    if (ImGui::InputText("##name", nameBuf, IM_ARRAYSIZE(nameBuf))) obj->name = nameBuf;
    ImGui::SameLine();

    if (ImGui::Button("Usun"))
        obj->pendingDelete = true;
    ImGui::SameLine();

    if (ImGui::TreeNode("Edytuj"))
    {
        ImGui::Indent();
        ImGui::ColorEdit3("Kolor", obj->color);

        if (!std::dynamic_pointer_cast<SceneBezierC0>(obj))
        {
            ImGui::Text("Transformacja obiektu:");
            ImGui::DragFloat3("Pozycja (XYZ)", &obj->transformations.posX, 0.1f, min_pos, max_pos);
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

        if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj))
        {
            ImGui::Checkbox("Pokaz lamana", &b->showPolygon);
            if (ImGui::Button("Dodaj nowe punkty"))
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
                        it = b->points.erase(it);
                    else ++it;

                    ImGui::PopID();
                }
                else
                {
                    it = b->points.erase(it);
                }
            }
        }
        else if (auto t = std::dynamic_pointer_cast<SceneTorus>(obj))
        {
            ImGui::Text("Geometria Torusa:");
            bool needsUpdate = false;
            needsUpdate |= ImGui::SliderFloat("R", &t->R, min_R, max_R);
            needsUpdate |= ImGui::SliderFloat("r", &t->r, min_r, max_r);
            needsUpdate |= ImGui::SliderInt("density R", &t->density_R, min_density_R, max_density_R);
            needsUpdate |= ImGui::SliderInt("density r", &t->density_r, min_density_r, max_density_r);
            if (needsUpdate)
                t->UpdateBuffers();
        }
        else if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj))
        {
            ImGui::Text("Geometria Punktu:");
            ImGui::SliderFloat("Rozmiar", &p->size, 1.0f, 20.0f);
        }
        ImGui::Unindent();
        ImGui::Separator();
        ImGui::TreePop();
    }
    ImGui::PopID();
}