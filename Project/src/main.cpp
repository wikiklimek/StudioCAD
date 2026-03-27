#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>


#define _USE_MATH_DEFINES

float const PI = (float)M_PI;




#include <MG1Math/Vect3.h>
#include <MG1Math/Vect4.h>
#include <MG1Math/Mat3.h>
#include <MG1Math/Mat4.h>

#include "shader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include "torusGrid.h"
#include "matrixesModelViewProjection.h"
#include "axis.h"
#include "cursor.h"
#include "sceneTorus.h"
#include "screenInteractions.h"
#include "applyTransformations.h"
#include "scenePoint.h"
#include "bakeTransform.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


enum DragMode { NONE, TRANSLATE, ROTATE_X, ROTATE_Y, ROTATE_Z, SCALE, ROTATE_FREE };

int main()
{

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //antyaliasing
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Torus", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;


    glEnable(GL_DEPTH_TEST);

    // wygładzanie krawędzi
    glEnable(GL_MULTISAMPLE);
    //glLineWidth(1.2f);


    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    Shader shader("src/shaders/torus.vs", "src/shaders/torus.fs");


    Cursor cursor;
    cursor.Init();
    std::vector<std::shared_ptr<SceneObject>> sceneObjects;


    int transformMode = 0; // 0 = lokalne, 1 = wspólny srodek, 2 = kursor, 3 - cala scena
    Vect3 centerOfSelection(0.0f, 0.0f, 0.0f);
    Transformations groupTransform;
    Vect3 centerOfTransformations(0.0f, 0.0f, 0.0f);
    bool isTransformationActive = false;


    auto firstTorus = std::make_shared<SceneTorus>("Torus Startowy", cursor.transform);
    firstTorus->Init();
    sceneObjects.push_back(firstTorus);


    int winWidth = 1024, winHeight = 768;

    float min_camera_distance_view = 0.1f;
    float max_camera_distance_view = 100.0f;

    float fov = PI / 4.0f;
    float aspectRatio = (float)winWidth/(float)winHeight;

    Vect3 cameraPos = Vect3(5.0f, -25.0f, 20.0f);
    Vect3 target = Vect3(0.0f, 0.0f, 0.0f);
    Vect3 up = Vect3(0.0f, 0.0f, 1.0f);


    bool isBoxSelecting = false;
    double boxStartX = 0, boxStartY = 0;
    double boxEndX = 0, boxEndY = 0;
    double boxSmallestXY = 5.0;

    bool isDragging = false;
    DragMode currentMode = NONE;
    double lastMouseX = 0, lastMouseY = 0;


    float min_pos = -50.0f;
    float max_pos = 50.0f;

    float min_scale = 0.01f;
    float max_scale = 10.0f;

    float min_axis = -1.0f;
    float max_axis = 1.0f;

    float min_angle = -360;
    float max_angle = 360;

    float max_R = 10.0f;
    float min_R = 0.1f;
    float max_r = 5.0f;
    float min_r = 0.1f;

    int max_density_R = 80;
    int min_density_R = 3;
    int max_density_r = 80;
    int min_density_r = 3;


    // OSIE OBROTU
    float axisBidirectionalVertices[] = {
            -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, // X
            0.0f,-1.0f, 0.0f,  0.0f, 1.0f, 0.0f, // Y
            0.0f, 0.0f,-1.0f,  0.0f, 0.0f, 1.0f  // Z
    };

    unsigned int localAxisVAO, localAxisVBO;
    glGenVertexArrays(1, &localAxisVAO);
    glGenBuffers(1, &localAxisVBO);
    glBindVertexArray(localAxisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, localAxisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisBidirectionalVertices), axisBidirectionalVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    bool drawAxesEuler = true;


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");



    int inputMode = 0; // 0 = Mysz, 1 = GUI
    int prevInputMode = 0;

    float guiDeltaPos[3] = {0.0f, 0.0f, 0.0f};
    float guiDeltaScale = 1.0f;

    int guiRotMode = 0; // 0 = Oś+Kąt, 1 = Kwaternion
    int prevGuiRotMode = 0;

    float guiRotAxis[3] = {0.0f, 1.0f, 0.0f};
    float guiRotAngle = 0.0f;
    float guiRotQuat[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // W, X, Y, Z

    // zerowanie transformacji gloablnych z gui
    auto clearGuiState = [&]() {
        guiDeltaPos[0] = guiDeltaPos[1] = guiDeltaPos[2] = 0.0f;
        guiDeltaScale = 1.0f;
        guiRotAxis[0] = 0.0f; guiRotAxis[1] = 1.0f; guiRotAxis[2] = 0.0f;
        guiRotAngle = 0.0f;
        guiRotQuat[0] = 1.0f; guiRotQuat[1] = 0.0f; guiRotQuat[2] = 0.0f; guiRotQuat[3] = 0.0f;
    };




    while (!glfwWindowShouldClose(window))
    {
        // rozmiar okna
        int currentW, currentH;
        glfwGetFramebufferSize(window, &currentW, &currentH);
        if (currentW > 0 && currentH > 0 && (currentW != winWidth || currentH != winHeight))
        {
            winWidth = currentW;
            winHeight = currentH;

            aspectRatio = (float)winWidth/(float)winHeight;
        }

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);



        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantCaptureMouse)
        {
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);

            bool isLeftClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            bool isRightClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

            // pozycja kursora
            if (isRightClick)
            {
                Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, cameraPos, target, up, fov);
                Vect3 intersection = getCursorIntersection(cameraPos, rayDir);
                cursor.transform.setPosition(intersection);
                cursor.screenX = (float)mouseX;
                cursor.screenY = (float)mouseY;
            }

            // Obsługa klawiszy trybów i kliknięć
            if (isLeftClick)
            {
                if (!isDragging)
                {
                    if (inputMode == 0 && glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) currentMode = TRANSLATE;
                    else if (inputMode == 0 && glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) currentMode = ROTATE_FREE;
                    else if (inputMode == 0 && glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) currentMode = ROTATE_X;
                    else if (inputMode == 0 && glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) currentMode = ROTATE_Y;
                    else if (inputMode == 0 && glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) currentMode = ROTATE_Z;
                    else if (inputMode == 0 && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) currentMode = SCALE;
                    else
                    {
                        currentMode = NONE;

                        // start rysowania ramki
                        isBoxSelecting = true;
                        boxStartX = mouseX;
                        boxStartY = mouseY;
                        boxEndX = mouseX;
                        boxEndY = mouseY;
                    }


                    if (currentMode != NONE)
                    {
                        isTransformationActive = true;
                        if (transformMode == 1)
                            centerOfTransformations = centerOfSelection;
                        else if (transformMode == 2)
                            centerOfTransformations = cursor.transform.getPosition();
                        else if (transformMode == 3)
                            centerOfTransformations = Vect3(0.0f, 0.0f, 0.0f);
                    }
                }


                if (isBoxSelecting)
                {
                    boxEndX = mouseX;
                    boxEndY = mouseY;
                }

                isDragging = true;
            }
            else //musi sie wykonac tylko raz po puszczeniu klawisza! dbac o to!
            {

                if (isBoxSelecting)
                {
                    isBoxSelecting = false;

                    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS &&
                        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) != GLFW_PRESS)
                        for (auto& obj : sceneObjects)
                            obj->isSelected = false;

                    // jezeli select box jest malutki to mamy klikniecie (a nie select boxa)
                    if (std::abs(boxEndX - boxStartX) < boxSmallestXY && std::abs(boxEndY - boxStartY) < boxSmallestXY)
                    {
                        Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, cameraPos, target, up, fov);
                        float minDist = 10000.0f;
                        std::shared_ptr<SceneObject> closestObj = nullptr;

                        for (auto& obj : sceneObjects)
                        {
                            if (!std::dynamic_pointer_cast<ScenePoint>(obj))
                                continue;

                            Vect3 objPos = obj->transformations.getPosition();
                            Vect3 toObj = objPos - cameraPos;
                            float projLength = Vect3::dot(toObj, rayDir);

                            if (projLength > 0.0f)
                            {
                                Vect3 projPoint = cameraPos + Vect3(rayDir.x * projLength, rayDir.y * projLength, rayDir.z * projLength);
                                float distToRay = std::sqrt((objPos.x - projPoint.x) * (objPos.x - projPoint.x) +
                                                                    (objPos.y - projPoint.y) * (objPos.y - projPoint.y) +
                                                                    (objPos.z - projPoint.z) * (objPos.z - projPoint.z));

                                if (distToRay < 1.5f && projLength < minDist)
                                {
                                    minDist = projLength;
                                    closestObj = obj;
                                }
                            }
                        }

                        if (closestObj != nullptr)
                            closestObj->isSelected = !closestObj->isSelected;
                    }
                    else //select box
                    {
                        // bez macierzy modelu bo i tak bierzemy opotem pozycje srodka - czyli przesuniecie punktu
                        Mat4 M_View  = createViewMatrix(cameraPos, target, up);
                        Mat4 M_Proj  = createProjectionMatrix(fov, aspectRatio, min_camera_distance_view, max_camera_distance_view);
                        Mat4 VP = M_Proj * M_View;


                        float minX = (float)std::min(boxStartX, boxEndX);
                        float maxX = (float)std::max(boxStartX, boxEndX);
                        float minY = (float)std::min(boxStartY, boxEndY);
                        float maxY = (float)std::max(boxStartY, boxEndY);

                        for (auto& obj : sceneObjects)
                        {
                            Vect3 pos = obj->transformations.getPosition();
                            Vect4 pos4(pos.x, pos.y, pos.z, 1.0f);
                            Vect4 clipSpace = VP * pos4;

                            // w - odleglosc od kamery
                            if (clipSpace.w > 0.0001f)
                            {
                                // [-1, 1]
                                Vect3 ndc(clipSpace.x / clipSpace.w, clipSpace.y / clipSpace.w, clipSpace.z / clipSpace.w);

                                float screenX = (ndc.x + 1.0f) / 2.0f * (float)winWidth;
                                float screenY = (1.0f - ndc.y) / 2.0f * (float)winHeight;


                                if (screenX >= minX && screenX <= maxX && screenY >= minY && screenY <= maxY)
                                {
                                    obj->isSelected = true;
                                }
                            }
                        }
                    }
                }

                // baking transformation to objects
                if (isTransformationActive)
                {
                    if (transformMode == 1 || transformMode == 2 || transformMode == 3)
                        bakeGroupTransform(sceneObjects, groupTransform, centerOfTransformations, transformMode == 3);

                    groupTransform = Transformations();
                    isTransformationActive = false;
                }

                isDragging = false;
                currentMode = NONE;
            }


            if (isDragging && currentMode != NONE)
            {
                if (mouseX != lastMouseX || mouseY != lastMouseY)
                {
                    float dx_screen = (float)(mouseX - lastMouseX);
                    float dy_screen = (float)(mouseY - lastMouseY);


                    float dx_world = (dx_screen / (float)winWidth) * 2.0f * aspectRatio;
                    float dy_world = (dy_screen / (float)winHeight) * 2.0f;


                    Vect3 deltaPos(0.0f, 0.0f, 0.0f);
                    float deltaScale = 1.0f;
                    Quaternion deltaQuat;


                    if (currentMode == TRANSLATE)
                    {
                        deltaPos.x = 15.0f * dx_world;
                        deltaPos.y = -15.0f * dy_world;
                    }
                    else if (currentMode == ROTATE_X)
                    {
                        deltaQuat = Quaternion::fromAxisAngle(1.0f, 0.0f, 0.0f, dy_world * 2.0f);
                    }
                    else if (currentMode == ROTATE_Y)
                    {
                        deltaQuat = Quaternion::fromAxisAngle(0.0f, 1.0f, 0.0f, dx_world * 2.0f);
                    }
                    else if (currentMode == ROTATE_Z)
                    {
                        deltaQuat = Quaternion::fromAxisAngle(0.0f, 0.0f, 1.0f, dx_world * 2.0f);
                    }
                    else if (currentMode == SCALE)
                    {
                        deltaScale = std::max(0.01f, 1.0f + (dx_world - dy_world) * 0.9f);
                    }
                    else if (currentMode == ROTATE_FREE)
                    {
                        float angle = std::sqrt(dx_world * dx_world + dy_world * dy_world) * 2.0f;

                        if (angle > 0.0001f)
                        {
                            deltaQuat = Quaternion::fromAxisAngle(dy_world, dx_world, 0.0f, angle);
                        }
                    }


                    if (transformMode == 0) // lolaklny tryb, zmiany nanoszone od razu do obiektow
                    {
                        for (auto& obj : sceneObjects)
                        {
                            if (!obj->isSelected) continue;

                            obj->transformations.posX += deltaPos.x;
                            obj->transformations.posY += deltaPos.y;
                            obj->transformations.posZ += deltaPos.z;

                            obj->transformations.scale *= deltaScale;

                            obj->transformations.rotation = deltaQuat * obj->transformations.rotation;
                            obj->transformations.rotation.normalize();
                        }
                    }
                    else // zmiany nanoszone do strukttury globalnej grupy zaznaczonej
                    {
                        groupTransform.posX += deltaPos.x;
                        groupTransform.posY += deltaPos.y;
                        groupTransform.posZ += deltaPos.z;

                        groupTransform.rotation = deltaQuat * groupTransform.rotation;
                        groupTransform.rotation.normalize();

                        groupTransform.scale *= deltaScale;
                    }
                }
            }
            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ImGui::Begin("Sterowanie i Obiekty");

        if (isBoxSelecting)
        {
            ImGui::GetForegroundDrawList()->AddRect(
                    ImVec2((float)boxStartX, (float)boxStartY),
                    ImVec2((float)boxEndX, (float)boxEndY),
                    IM_COL32(255, 255, 255, 255),
                    0.0f, 0, 1.5f
            );
        }


        ImGui::Text("Kursor");
        ImGui::DragFloat3("Pozycja (Scena)", &cursor.transform.posX, 0.1f, min_pos, max_pos);


        //float cursorQuat[4] = {cursor.transform.rotation.w, cursor.transform.rotation.x, cursor.transform.rotation.y, cursor.transform.rotation.z};
        //if (ImGui::DragFloat4("Rotacja (WXYZ)##Cursor", cursorQuat, 0.01f, -1.0f, 1.0f)) {
        //    cursor.transform.rotation.w = cursorQuat[0];
        //    cursor.transform.rotation.x = cursorQuat[1];
        //    cursor.transform.rotation.y = cursorQuat[2];
        //    cursor.transform.rotation.z = cursorQuat[3];
        //    cursor.transform.rotation.normalize();
        // }

        ImGui::Text("Pozycja (Ekran): X: %.1f, Y: %.1f", cursor.screenX, cursor.screenY);
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
        }
        ImGui::SameLine();
        if (ImGui::Button("Usun Zaznacz."))
        {
            sceneObjects.erase(std::remove_if(sceneObjects.begin(), sceneObjects.end(),
                                              [](const std::shared_ptr<SceneObject>& o) { return o->isSelected; }), sceneObjects.end());
        }
        ImGui::Separator();


        centerOfSelection = Vect3(0,0,0);
        int selCount = 0;
        for (auto& obj : sceneObjects)
        {
            if (obj->isSelected)
            {
                centerOfSelection += obj->transformations.getPosition();
                selCount++;
            }
        }

        if (selCount > 0)
            centerOfSelection = Vect3(centerOfSelection.x/selCount, centerOfSelection.y/selCount, centerOfSelection.z/selCount);

        ImGui::Text("Zaznaczonych: %d | Srodek: %.1f, %.1f, %.1f", selCount, centerOfSelection.x, centerOfSelection.y, centerOfSelection.z);

        ImGui::RadioButton("Zaznacz. Lokalne", &transformMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Wspolny Środek", &transformMode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Wzgledem Kursora", &transformMode, 2);
        ImGui::SameLine();
        ImGui::RadioButton("Cala Scena", &transformMode, 3);

        ImGui::Separator();


        ImGui::Text("Metoda transformacji:");

        if (ImGui::RadioButton("Mysz (Skróty klawiszowe)", &inputMode, 0) && prevInputMode == 1)
        {
            clearGuiState();
        }
        ImGui::SameLine();
        ImGui::RadioButton("Wartosci z GUI (Live Preview)", &inputMode, 1);
        prevInputMode = inputMode;


        if (inputMode == 1)
        {
            ImGui::Separator();
            ImGui::DragFloat3("Przesuniecie (XYZ)", guiDeltaPos, 0.1f, min_pos, max_pos);
            if (ImGui::DragFloat("Skala (Mnoznik)", &guiDeltaScale, 0.05f, min_scale, max_scale))
            {
                guiDeltaScale = std::max(0.01f, guiDeltaScale);
            }

            ImGui::Text("Tryb Rotacji:");
            ImGui::RadioButton("Os i Kat", &guiRotMode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Kwaternion", &guiRotMode, 1);

            // zmiana totacji <-> zerowanie rotacji
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
                    // wektor obrotu NIE może być równy (0,0,0)
                    if (std::abs(guiRotAxis[0]) < 0.0001f &&
                        std::abs(guiRotAxis[1]) < 0.0001f &&
                        std::abs(guiRotAxis[2]) < 0.0001f) {
                        guiRotAxis[1] = 1.0f;
                    }
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
                {
                    previewQuat = Quaternion::fromAxisAngle(guiRotAxis[0], guiRotAxis[1], guiRotAxis[2], guiRotAngle * PI / 180.0f);
                }
                else if (guiRotMode == 1)
                {
                    previewQuat = Quaternion(guiRotQuat[0], guiRotQuat[1], guiRotQuat[2], guiRotQuat[3]);
                }
                previewQuat.normalize();


                if (transformMode == 0) // local - aplikujemy bezpośrednio na obiekty
                {
                    for (auto& obj : sceneObjects)
                    {
                        if (obj->isSelected)
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
                    // GRUPOWE - używamy funkcji bakeGroupTransform
                    Vect3 center = (transformMode == 3) ? Vect3(0,0,0) : ((transformMode == 2) ? cursor.transform.getPosition() : centerOfSelection);

                    Transformations tempGroup;
                    tempGroup.posX = guiDeltaPos[0];
                    tempGroup.posY = guiDeltaPos[1];
                    tempGroup.posZ = guiDeltaPos[2];
                    tempGroup.scale = guiDeltaScale;
                    tempGroup.rotation = previewQuat;


                    bakeGroupTransform(sceneObjects, tempGroup, center, transformMode == 3);
                }
                clearGuiState();
            }

            ImGui::SameLine();
            if (ImGui::Button("Wyczysc", ImVec2(100, 30)))
            {
                clearGuiState();
            }
        }




        ImGui::Separator();
        ImGui::Text("Lista Obiektow na Scenie:");
        for (auto& obj : sceneObjects)
        {
            ImGui::PushID(obj.get());
            ImGui::Checkbox("##sel", &obj->isSelected);
            ImGui::SameLine();

            char nameBuf[128];
            strcpy(nameBuf, obj->name.c_str());
            if (ImGui::InputText("##name", nameBuf, IM_ARRAYSIZE(nameBuf))) obj->name = nameBuf;


            if (obj->isSelected)
            {
                ImGui::Indent();

                ImGui::ColorEdit3("Kolor", obj->color);

                ImGui::Text("Transformacja obiektu:");
                ImGui::DragFloat3("Pozycja (XYZ)", &obj->transformations.posX, 0.1f, min_pos, max_pos);
                ImGui::DragFloat("Skala", &obj->transformations.scale, 0.05f, min_scale, max_scale);

                float quatVals[4] = {obj->transformations.rotation.w, obj->transformations.rotation.x, obj->transformations.rotation.y, obj->transformations.rotation.z};
                if (ImGui::DragFloat4("Rotacja (WXYZ)", quatVals, 0.01f, -1.0f, 1.0f))
                {
                    obj->transformations.rotation.w = quatVals[0];
                    obj->transformations.rotation.x = quatVals[1];
                    obj->transformations.rotation.y = quatVals[2];
                    obj->transformations.rotation.z = quatVals[3];
                    obj->transformations.rotation.normalize();
                }


                if (auto t = std::dynamic_pointer_cast<SceneTorus>(obj))
                {
                    ImGui::Text("Geometria Torusa:");
                    bool needsUpdate = false;
                    needsUpdate |= ImGui::SliderFloat("R", &t->R, min_R, max_R);
                    needsUpdate |= ImGui::SliderFloat("r", &t->r, min_r, max_r);
                    needsUpdate |= ImGui::SliderInt("density R", &t->density_R, min_density_R, max_density_R);
                    needsUpdate |= ImGui::SliderInt("density r", &t->density_r, min_density_r, max_density_r);
                    if (needsUpdate)
                    {
                        t->UpdateBuffers();
                    }
                }

                ImGui::Unindent();
                ImGui::Separator();
            }
            ImGui::PopID();
        }
        ImGui::End();


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        Mat4 M_View  = createViewMatrix(cameraPos, target, up);
        Mat4 M_Proj  = createProjectionMatrix(fov, aspectRatio, min_camera_distance_view, max_camera_distance_view );

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, M_View.table);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, M_Proj.table);




        Mat4 M_group_mouse(1.0f);
        Mat4 M_group_gui(1.0f);


        Quaternion previewDeltaQuat(1.0f, 0.0f, 0.0f, 0.0f);
        if (inputMode == 1)
        {
            if (guiRotMode == 0)
            {
                previewDeltaQuat = Quaternion::fromAxisAngle(guiRotAxis[0], guiRotAxis[1], guiRotAxis[2], guiRotAngle * PI / 180.0f);
            }
            else if (guiRotMode == 1)
            {
                previewDeltaQuat = Quaternion(guiRotQuat[0], guiRotQuat[1], guiRotQuat[2], guiRotQuat[3]);
            }
            previewDeltaQuat.normalize();
        }


        bool applyMousePreview = (inputMode == 0 && isTransformationActive && (transformMode == 1 || transformMode == 2 || transformMode == 3));
        if (applyMousePreview)
        {
            Mat4 T_toOrigin = Mat4::translate_inverse(centerOfTransformations);
            Mat4 R_group = groupTransform.rotation.toMat4();
            Mat4 S_group = Mat4::scale(Vect3(groupTransform.scale, groupTransform.scale, groupTransform.scale));
            Mat4 T_toPos = Mat4::translate(centerOfTransformations + groupTransform.getPosition());
            M_group_mouse = T_toPos * R_group * S_group * T_toOrigin;
        }


        bool applyGuiPreviewGroup = (inputMode == 1 && transformMode != 0);
        if (applyGuiPreviewGroup)
        {
            Vect3 center = (transformMode == 3) ? Vect3(0,0,0) : ((transformMode == 2) ? cursor.transform.getPosition() : centerOfSelection);
            Mat4 T_toOrigin = Mat4::translate_inverse(center);
            Mat4 R_group = previewDeltaQuat.toMat4();
            Mat4 S_group = Mat4::scale(Vect3(guiDeltaScale, guiDeltaScale, guiDeltaScale));
            Mat4 T_toPos = Mat4::translate(center + Vect3(guiDeltaPos[0], guiDeltaPos[1], guiDeltaPos[2]));
            M_group_gui = T_toPos * R_group * S_group * T_toOrigin;
        }


        for (auto& obj : sceneObjects)
        {
            bool isTargetGroup = (transformMode == 3 || obj->isSelected);
            bool isTargetLocal = (transformMode == 0 && obj->isSelected);

            if (inputMode == 1 && isTargetLocal) //GUI, lokalnie, wybrane
            {
                Quaternion oldRot = obj->transformations.rotation;

                // Modifikacja
                obj->transformations.posX += guiDeltaPos[0];
                obj->transformations.posY += guiDeltaPos[1];
                obj->transformations.posZ += guiDeltaPos[2];
                obj->transformations.scale *= guiDeltaScale;

                obj->transformations.rotation = previewDeltaQuat * obj->transformations.rotation;
                obj->transformations.rotation.normalize();


                //obj->Draw(shader, Mat4(1.0f));
                obj->Draw(shader);

                // Cofa Modifikację
                obj->transformations.posX -= guiDeltaPos[0];
                obj->transformations.posY -= guiDeltaPos[1];
                obj->transformations.posZ -= guiDeltaPos[2];
                obj->transformations.scale /= guiDeltaScale;

                obj->transformations.rotation = oldRot;
            }
            else if (applyGuiPreviewGroup && isTargetGroup) //gui, wybrane - czyli preview
            {
                obj->Draw(shader, M_group_gui);
            }
            else if (applyMousePreview && isTargetGroup) //mysz, wybrane - prewiev
            {
                obj->Draw(shader, M_group_mouse);
            }
            else // bez preview
            {
                //obj->Draw(shader, Mat4(1.0f));
                obj->Draw(shader);
            }
        }


        cursor.Draw(shader);


        if(drawAxesEuler)
        {
            float dummyPos[3] = {0,0,0};
            float dummyRot[3] = {0,0,0};
            drawEulerAxes(shader, localAxisVAO, dummyPos, dummyRot, 100.0f);
        }



        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

    }


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}