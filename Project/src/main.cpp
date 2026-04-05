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
#include "camera.h"
#include "sceneBezierC0.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


enum DragMode { BOX, TRANSLATE, ROTATE_X, ROTATE_Y, ROTATE_Z, SCALE, ROTATE_FREE };
enum CameraDragMode { CAM_NONE, CAM_ORBIT, CAM_ZOOM, CAM_PAN };


enum TransformMode { LOCAL, COMMON_CENTER, CURSOR_CENTER, ENTIRE_SCENE };
enum InputMode { INPUT_MOUSE, INPUT_GUI };

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

    Shader bezierShader("src/shaders/bezier.vs", "src/shaders/bezier.fs");

    bool magicMode = false;
    std::shared_ptr<SceneBezierC0> magicCurve = nullptr;


    Cursor cursor;
    cursor.Init();
    std::vector<std::shared_ptr<SceneObject>> sceneObjects;


    TransformMode transformMode = LOCAL; // 0 = lokalne, 1 = wspólny srodek, 2 = kursor, 3 - cala scena
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

    Camera camera(Vect3(5.0f, -25.0f, 20.0f), Vect3(0.0f, 0.0f, 0.0f), Vect3(0.0f, 0.0f, 1.0f));

    CameraDragMode camMode = CAM_NONE;
    bool isCamDragging = false;
    double startCamMouseX = 0, startCamMouseY = 0;

    float minAngleCameraZ = 0.05f;
    float maxAngleCameraZ = PI - 0.05f;


    bool isBoxSelecting = false;
    double boxStartX = 0, boxStartY = 0;
    double boxEndX = 0, boxEndY = 0;
    double boxSmallestXY = 5.0;

    bool isDragging = false;
    DragMode currentMode = BOX;
    double lastMouseX = 0, lastMouseY = 0;
    double startMouseX = 0, startMouseY = 0;


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



    InputMode inputMode = INPUT_MOUSE; // 0 = Mysz, 1 = GUI
    InputMode prevInputMode = INPUT_MOUSE;

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


            if (isRightClick)
            {
                if (!isCamDragging)
                {
                    if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
                        camMode = CAM_ORBIT;
                    else if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                        camMode = CAM_ZOOM;
                    else if(glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
                        camMode = CAM_PAN;

                    if (camMode == CAM_ORBIT || camMode == CAM_ZOOM || camMode ==CAM_PAN)
                    {
                        isCamDragging = true;
                        camera.hasTemporaryChanges = true;
                        startCamMouseX = mouseX;
                        startCamMouseY = mouseY;
                    }
                    else // przestaw kursor 3D
                    {
                        Vect3 activeCamPos(0.0), activeCamTarget(0.0);
                        camera.getActiveState(activeCamPos, activeCamTarget);

                        Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, activeCamPos, activeCamTarget, camera.up, fov);
                        Vect3 intersection = getCursorIntersection(activeCamPos, rayDir);

                        cursor.transform.setPosition(intersection);
                        cursor.screenX = (float)mouseX;
                        cursor.screenY = (float)mouseY;
                    }
                }
            }
            else
            {
                // Puszczenie prawego przycisku - wypiekamy kamerę
                if (isCamDragging)
                {
                    camera.bake();
                    isCamDragging = false;
                    camMode = CAM_NONE;
                }
            }


            if (isCamDragging)
            {
                float dx_screen = (float)(mouseX - startCamMouseX);
                float dy_screen = (float)(mouseY - startCamMouseY);

                float dx_world = (dx_screen / (float)winWidth) * 2.0f;
                float dy_world = (dy_screen / (float)winHeight) * 2.0f;

                if (camMode == CAM_ORBIT)
                {
                    Quaternion qZ = Quaternion::fromAxisAngle(0.0f, 0.0f, 1.0f, -dx_world * 2.0f);

                    Vect3 dir = camera.getDirectionNotBaked().normalize();
                    Vect3 right = Vect3::cross(camera.up, dir).normalize();

                    float currentAngleZ = std::acos(dir.z);
                    float deltaAngleZ = -dy_world * 2.0f;


                    //zeby kamera nie byla pionowo/poziomo
                    if (currentAngleZ + deltaAngleZ < minAngleCameraZ)
                    {
                        deltaAngleZ = minAngleCameraZ - currentAngleZ;
                    }
                    else if (currentAngleZ + deltaAngleZ > maxAngleCameraZ)
                    {
                        deltaAngleZ = maxAngleCameraZ - currentAngleZ;
                    }


                    Quaternion qRight = Quaternion::fromAxisAngle(right.x, right.y, right.z, deltaAngleZ);

                    // skladanie kwaternionow
                    camera.tempOrbit = qRight * qZ;
                    camera.tempOrbit.normalize();
                }
                else if (camMode == CAM_ZOOM)
                {
                    Vect3 dir = camera.getDirectionNotBaked();
                    // Obliczamy prawdziwy dystans 3D
                    float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

                    // Bezpiecznik: Minimalny współczynnik zoomu zapobiegający wejściu w target
                    float minZoom = 0.1f / dist;

                    // w górę przybliża, w dół oddala (blokujemy przed wejściem na <0.1 dystansu)
                    camera.tempZoom = std::max(minZoom, 1.0f - dy_world * 2.0f);
                }
                else if (camMode == CAM_PAN)
                {
                    Vect3 dir = camera.getDirectionNotBaked().normalize();
                    Vect3 right = Vect3::cross(camera.up, dir).normalize();
                    Vect3 localUp = Vect3::cross(dir, right).normalize();

                    Vect3 direction = camera.getDirectionNotBaked();

                    //dla zachowania odleglosci kamery
                    float distance = std::sqrt(direction.x * direction.x +
                            direction.y * direction.y +
                            direction.z * direction.z);

                    Vect3 panRight = Vect3(right.x * -dx_world * distance, right.y * -dx_world * distance, right.z * -dx_world * distance);
                    Vect3 panUp = Vect3(localUp.x * dy_world * distance, localUp.y * dy_world * distance, localUp.z * dy_world * distance);

                    camera.tempPan = panRight + panUp;
                }
            }

            // Obsługa klawiszy trybów i kliknięć
            if (isLeftClick)
            {
                if (magicMode)
                {
                    if (!isDragging) // Uruchomi się tylko raz na kliknięcie
                    {
                        // 1. Zaktualizuj pozycję kursora natychmiastowo
                        Vect3 activeCamPos(0.0), activeCamTarget(0.0);
                        camera.getActiveState(activeCamPos, activeCamTarget);
                        Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, activeCamPos, activeCamTarget, camera.up, fov);
                        Vect3 intersection = getCursorIntersection(activeCamPos, rayDir);
                        cursor.transform.setPosition(intersection);
                        cursor.screenX = (float)mouseX;
                        cursor.screenY = (float)mouseY;

                        // 2. Stwórz punkt i dodaj do magicznej krzywej
                        auto p = std::make_shared<ScenePoint>("Punkt " + std::to_string(sceneObjects.size()+1), cursor.transform);
                        p->Init();
                        sceneObjects.push_back(p);
                        magicCurve->points.push_back(p);
                    }
                    isDragging = true;
                }
                else if (!isDragging)
                {
                    if (inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) currentMode = TRANSLATE;
                    else if (inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) currentMode = ROTATE_FREE;
                    else if (inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) currentMode = ROTATE_X;
                    else if (inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) currentMode = ROTATE_Y;
                    else if (inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) currentMode = ROTATE_Z;
                    else if (inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) currentMode = SCALE;
                    else
                    {
                        currentMode = BOX;

                        // start rysowania ramki
                        isBoxSelecting = true;
                        boxStartX = mouseX;
                        boxStartY = mouseY;
                        boxEndX = mouseX;
                        boxEndY = mouseY;
                    }


                    if (currentMode != BOX)
                    {
                        isTransformationActive = true;
                        startMouseX = mouseX;
                        startMouseY = mouseY;
                        groupTransform = Transformations();


                        if (transformMode == COMMON_CENTER)
                            centerOfTransformations = centerOfSelection;
                        else if (transformMode == CURSOR_CENTER)
                            centerOfTransformations = cursor.transform.getPosition();
                        else if (transformMode == ENTIRE_SCENE)
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

                    // Box odznacza WSZYSTKIE krzywe beziera (chyba że mamy Shift, ale prosiłaś bez)
                    for(auto& obj : sceneObjects) {
                        if (std::dynamic_pointer_cast<SceneBezierC0>(obj)) {
                            obj->isSelected = false;
                        }
                    }

                    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS &&
                        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) != GLFW_PRESS)
                        for (auto& obj : sceneObjects)
                            obj->isSelected = false;

                    // jezeli select box jest malutki to mamy klikniecie (a nie select boxa)
                    if (std::abs(boxEndX - boxStartX) < boxSmallestXY && std::abs(boxEndY - boxStartY) < boxSmallestXY)
                    {
                        Vect3 activeCamPos(0.0), activeCamTarget(0.0);
                        camera.getActiveState(activeCamPos, activeCamTarget);

                        Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, activeCamPos, activeCamTarget, camera.up, fov);
                        float minDist = 10000.0f;
                        std::shared_ptr<SceneObject> closestObj = nullptr;

                        for (auto& obj : sceneObjects)
                        {
                            if (!std::dynamic_pointer_cast<ScenePoint>(obj))
                                continue;


                            Vect3 objPos = obj->transformations.getPosition();
                            Vect3 toObj = objPos - activeCamPos;
                            float projLength = Vect3::dot(toObj, rayDir);

                            //czy obiekt jest "z przodu" kamery
                            if (projLength > 0.0f)
                            {
                                //rzut obiektu na prosta klikniecia
                                Vect3 projPoint = activeCamPos + Vect3(rayDir.x * projLength, rayDir.y * projLength, rayDir.z * projLength);
                                float distToRay = std::sqrt((objPos.x - projPoint.x) * (objPos.x - projPoint.x) +
                                                                    (objPos.y - projPoint.y) * (objPos.y - projPoint.y) +
                                                                    (objPos.z - projPoint.z) * (objPos.z - projPoint.z));

                                // lapiemy ten bardziej z przodu
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
                        Mat4 M_View = camera.getViewMatrix();
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
                    if (transformMode == LOCAL)
                    {
                        // Wypalanie dla trybu lokalnego
                        for (auto& obj : sceneObjects)
                        {
                            // NOWY KOD WYPIEKANIA LOCAL
                            if (std::dynamic_pointer_cast<SceneBezierC0>(obj)) continue; // Krzywe ignorujemy

                            bool shouldBake = obj->isSelected;
                            if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj)) {
                                if (p->selectedCurvesCount > 0) shouldBake = true;
                            }

                            if (!shouldBake) continue;

                            obj->transformations.posX += groupTransform.posX;
                            obj->transformations.posY += groupTransform.posY;
                            obj->transformations.posZ += groupTransform.posZ;
                            obj->transformations.scale *= groupTransform.scale;
                            obj->transformations.rotation = groupTransform.rotation * obj->transformations.rotation;
                            obj->transformations.rotation.normalize();
                        }
                    }
                    else if (transformMode == COMMON_CENTER || transformMode == CURSOR_CENTER || transformMode == ENTIRE_SCENE)
                    {
                        // Wypalanie dla trybów grupowych
                        bakeGroupTransform(sceneObjects, groupTransform, centerOfTransformations, transformMode == ENTIRE_SCENE);
                    }

                    groupTransform = Transformations();
                    isTransformationActive = false;
                }

                isDragging = false;
                currentMode = BOX;
            }


            if (isDragging && currentMode != BOX)
            {
                float dx_screen = (float)(mouseX - startMouseX);
                float dy_screen = (float)(mouseY - startMouseY);

                float dx_world = (dx_screen / (float)winWidth) * 2.0f * aspectRatio;
                float dy_world = (dy_screen / (float)winHeight) * 2.0f;

                // zerowanie transformacji
                groupTransform = Transformations();

                if (currentMode == TRANSLATE)
                {
                    groupTransform.posX = 15.0f * dx_world;
                    groupTransform.posY = -15.0f * dy_world;
                }
                else if (currentMode == ROTATE_X)
                {
                    groupTransform.rotation = Quaternion::fromAxisAngle(1.0f, 0.0f, 0.0f, dy_world * 2.0f);
                }
                else if (currentMode == ROTATE_Y)
                {
                    groupTransform.rotation = Quaternion::fromAxisAngle(0.0f, 1.0f, 0.0f, dx_world * 2.0f);
                }
                else if (currentMode == ROTATE_Z)
                {
                    groupTransform.rotation = Quaternion::fromAxisAngle(0.0f, 0.0f, 1.0f, dx_world * 2.0f);
                }
                else if (currentMode == SCALE)
                {
                    groupTransform.scale = std::max(0.01f, 1.0f + (dx_world - dy_world) * 0.9f);
                }
                else if (currentMode == ROTATE_FREE)
                {
                    float angle = std::sqrt(dx_world * dx_world + dy_world * dy_world) * 2.0f;
                    if (angle > 0.0001f)
                    {
                        groupTransform.rotation = Quaternion::fromAxisAngle(dy_world, dx_world, 0.0f, angle);
                    }
                }

            }
            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }



        // --- AKTUALIZACJA ZNACZNIKÓW PUNKTÓW ---
        for (auto& obj : sceneObjects) {
            if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj)) p->selectedCurvesCount = 0;
        }
        for (auto& obj : sceneObjects) {
            if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj)) {
                if (b->isSelected) {
                    for (auto& wp : b->points) {
                        if (auto p = wp.lock()) p->selectedCurvesCount++;
                    }
                }
            }
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

            // Auto-dodawanie do zaznaczonych krzywych (Drugi przypadek z Twojego opisu)
            for(auto& obj : sceneObjects) {
                if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj)) {
                    if (b->isSelected) b->points.push_back(p);
                }
            }
        }
        ImGui::SameLine();

        //zmianna w konkretnej klatce wiec lolanie deklaruje
        bool isGuiDisabledThisFrame = false;

        // Zmienna statyczna zapamiętująca nasz wybór z listy
        static int selectedBezierIndex = 0;

        if (!magicMode)
        {
            // 1. Zbieramy nazwy wszystkich dostępnych krzywych na scenie
            std::vector<std::string> bezierNames;
            std::vector<std::shared_ptr<SceneBezierC0>> bezierPointers;
            bezierNames.push_back("Nowy Bezier"); // Opcja domyślna

            for (auto& obj : sceneObjects) {
                if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj)) {
                    bezierNames.push_back(b->name);
                    bezierPointers.push_back(b);
                }
            }
            // Zabezpieczenie, gdybyśmy usunęli krzywą z listy
            if (selectedBezierIndex >= bezierNames.size()) selectedBezierIndex = 0;

            // 2. Rysujemy kontrolkę Combo Box
            ImGui::SetNextItemWidth(120);
            if (ImGui::BeginCombo("##bezierCombo", bezierNames[selectedBezierIndex].c_str()))
            {
                for (int i = 0; i < bezierNames.size(); i++) {
                    bool is_selected = (selectedBezierIndex == i);
                    if (ImGui::Selectable(bezierNames[i].c_str(), is_selected)) {
                        selectedBezierIndex = i;
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();

            // 3. Przycisk Akcji
            if (ImGui::Button("Stworz/Dodaj"))
            {
                std::vector<std::shared_ptr<ScenePoint>> selPts;
                for(auto& obj : sceneObjects) {
                    if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj)) {
                        if (p->isSelected) selPts.push_back(p);
                    }
                }

                if (selectedBezierIndex == 0)
                {   // TWORZYMY NOWĄ KRZYWĄ [cite: 8]
                    if (!selPts.empty()) {
                        auto b = std::make_shared<SceneBezierC0>("Bezier " + std::to_string(sceneObjects.size()+1), Transformations());
                        b->Init();
                        for(auto& p : selPts) b->points.push_back(p);
                        sceneObjects.push_back(b);
                        magicMode = true;
                        magicCurve = b;
                    }
                }
                else
                {   // DODAJEMY DO ISTNIEJĄCEJ KRZYWEJ [cite: 10, 13]
                    auto b = bezierPointers[selectedBezierIndex - 1];
                    for(auto& p : selPts) {
                        // Zabezpieczenie przed dublowaniem węzłów
                        bool exists = false;
                        for(auto& wp : b->points) {
                            if (wp.lock() == p) { exists = true; break; }
                        }
                        if (!exists) b->points.push_back(p);
                    }
                    magicMode = true;
                    magicCurve = b;
                }
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Zapisz zmiany (Krzywa)")) {
                magicMode = false;
                magicCurve = nullptr;
            }
            ImGui::PopStyleColor();

            // BLOKADA GUI PODCZAS MAGIC MODE
            ImGui::BeginDisabled();
            isGuiDisabledThisFrame = true; // <--- DODAJ TĘ LINIJKĘ
        }
        ImGui::SameLine();
        if (ImGui::Button("Usun Zaznacz."))
        {
            sceneObjects.erase(std::remove_if(sceneObjects.begin(), sceneObjects.end(),
                                              [](const std::shared_ptr<SceneObject>& o) { return o->isSelected; }), sceneObjects.end());
        }
        ImGui::Separator();


        centerOfSelection = Vect3(0,0,0);
        Vect3 globalCenter(0,0,0); // Dodajemy zmienną na środek ciężkości całej sceny
        int selCount = 0;

        for (auto& obj : sceneObjects)
        {
            globalCenter += obj->transformations.getPosition();
            if (obj->isSelected)
            {
                centerOfSelection += obj->transformations.getPosition();
                selCount++;
            }
        }

        if (selCount > 0)
            centerOfSelection = Vect3(centerOfSelection.x/selCount, centerOfSelection.y/selCount, centerOfSelection.z/selCount);

        if (!sceneObjects.empty())
            globalCenter = Vect3(globalCenter.x/sceneObjects.size(), globalCenter.y/sceneObjects.size(), globalCenter.z/sceneObjects.size());

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


        ImGui::RadioButton("Zaznacz. Lokalne", reinterpret_cast<int *>(&transformMode), LOCAL);
        ImGui::SameLine();
        ImGui::RadioButton("Wspolny Środek", reinterpret_cast<int *>(&transformMode), COMMON_CENTER);
        ImGui::SameLine();
        ImGui::RadioButton("Wzgledem Kursora", reinterpret_cast<int *>(&transformMode), CURSOR_CENTER);
        ImGui::SameLine();
        ImGui::RadioButton("Cala Scena", reinterpret_cast<int *>(&transformMode), ENTIRE_SCENE);

        ImGui::Separator();


        ImGui::Text("Metoda transformacji:");

        if (ImGui::RadioButton("Mysz (Skróty klawiszowe)", reinterpret_cast<int *>(&inputMode), INPUT_MOUSE) && prevInputMode == INPUT_GUI)
        {
            clearGuiState();
        }
        ImGui::SameLine();
        ImGui::RadioButton("Wartosci z GUI (Live Preview)", reinterpret_cast<int *>(&inputMode), INPUT_GUI);
        prevInputMode = inputMode;


        if (inputMode == INPUT_GUI)
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


                if (transformMode == LOCAL) // local - aplikujemy bezpośrednio na obiekty
                {
                    for (auto& obj : sceneObjects)
                    {
                        // NOWY KOD WYPIEKANIA LOCAL (GUI)
                        if (std::dynamic_pointer_cast<SceneBezierC0>(obj)) continue; // Krzywe ignorujemy

                        bool shouldBake = obj->isSelected;
                        if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj)) {
                            if (p->selectedCurvesCount > 0) shouldBake = true;
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
                    // GRUPOWE - używamy funkcji bakeGroupTransform
                    Vect3 center = (transformMode == ENTIRE_SCENE) ? Vect3(0,0,0) : ((transformMode == CURSOR_CENTER) ? cursor.transform.getPosition() : centerOfSelection);

                    Transformations tempGroup;
                    tempGroup.posX = guiDeltaPos[0];
                    tempGroup.posY = guiDeltaPos[1];
                    tempGroup.posZ = guiDeltaPos[2];
                    tempGroup.scale = guiDeltaScale;
                    tempGroup.rotation = previewQuat;


                    bakeGroupTransform(sceneObjects, tempGroup, center, transformMode == ENTIRE_SCENE);
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
        if (ImGui::CollapsingHeader("Sterowanie Kamerą"))
        {
            // Pobieramy "wypieczony" stan. Niepożądane jest edytowanie wartości z GUI
            // w trakcie przeciągania myszką (powodowałoby to skoki).
            if (!isCamDragging)
            {
                bool camChanged = false; // Flaga monitorująca zmiany
                camChanged |= ImGui::DragFloat3("Pozycja Kamery", &camera.position.x, 0.1f);
                camChanged |= ImGui::DragFloat3("Cel Kamery (Target)", &camera.target.x, 0.1f);

                if (ImGui::Button("Reset Kamery")) {
                    camera.position = Vect3(5.0f, -25.0f, 20.0f);
                    camera.target = Vect3(0.0f, 0.0f, 0.0f);
                    camChanged = true;
                }

                // ==========================================
                // ZABEZPIECZENIA WPROWADZANYCH DANYCH
                // ==========================================
                if (camChanged)
                {
                    Vect3 dir = camera.position - camera.target;
                    float dist = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);

                    // 1. Zabezpieczenie przed dystansem mniejszym niż 0.1 (Złączenie punktów)
                    if (dist < 0.1f)
                    {
                        if (dist < 0.0001f) dir = Vect3(0.0f, -1.0f, 0.0f); // Awaryjny wektor awarii
                        else { dir.x /= dist; dir.y /= dist; dir.z /= dist; }

                        camera.position = camera.target + Vect3(dir.x * 0.1f, dir.y * 0.1f, dir.z * 0.1f);
                        dir = camera.position - camera.target; // Ponowne przeliczenie po korekcie
                        dist = 0.1f;
                    }

                    // 2. Zabezpieczenie przed Gimbal Lock i znikaniem świata (Równoległość do osi Z)
                    dir.x /= dist; dir.y /= dist; dir.z /= dist; // Normalizacja do długości 1

                    // Iloczyn skalarny z wektorem UP (0,0,1). Jeśli |dot| == 1, patrzymy idealnie w dół/górę
                    float dot = dir.z;
                    if (std::abs(dot) > 0.999f)
                    {
                        // Dodajemy minimalny dryf do kamery, wybijając ją z niebezpiecznego bieguna
                        camera.position.x += 0.05f;
                        camera.position.y += 0.05f;
                    }
                }
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Edycja myszką w toku...");
            }
        }



        ImGui::Separator();
        ImGui::Text("Lista Obiektow na Scenie:");

        // rysowanie pojedynczego wiesza gui dla obiektow
        auto renderObjectGuiRow = [&](std::shared_ptr<SceneObject>& obj) {
            ImGui::PushID(obj.get());

            // Rysowanie znaczników dla Punktu
            if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj)) {
                if (p->selectedCurvesCount > 0) ImGui::TextColored(ImVec4(0, 1, 0, 1), "(O) %d ", p->selectedCurvesCount);
                else ImGui::TextColored(ImVec4(1, 0, 0, 1), "(O) 0 ");
                ImGui::SameLine();
            }

            ImGui::Checkbox("##sel", &obj->isSelected);
            ImGui::SameLine();

            char nameBuf[128];
            strcpy(nameBuf, obj->name.c_str());
            ImGui::SetNextItemWidth(150);
            if (ImGui::InputText("##name", nameBuf, IM_ARRAYSIZE(nameBuf))) obj->name = nameBuf;
            ImGui::SameLine();

            // NOWY PRZYCISK USUN (DLA WSZYSTKICH)
            if (ImGui::Button("Usun")) {
                obj->pendingDelete = true;
            }
            ImGui::SameLine();


            if (ImGui::TreeNode("Edytuj"))
            {
                ImGui::Indent();

                ImGui::ColorEdit3("Kolor", obj->color);

                if (!std::dynamic_pointer_cast<SceneBezierC0>(obj)) {
                    ImGui::Text("Transformacja obiektu:");
                    ImGui::DragFloat3("Pozycja (XYZ)", &obj->transformations.posX, 0.1f, min_pos, max_pos);
                    ImGui::DragFloat("Skala", &obj->transformations.scale, 0.05f, min_scale, max_scale);

                    float quatVals[4] = {obj->transformations.rotation.w, obj->transformations.rotation.x,
                                         obj->transformations.rotation.y, obj->transformations.rotation.z};
                    if (ImGui::DragFloat4("Rotacja (WXYZ)", quatVals, 0.01f, -1.0f, 1.0f)) {
                        obj->transformations.rotation.w = quatVals[0];
                        obj->transformations.rotation.x = quatVals[1];
                        obj->transformations.rotation.y = quatVals[2];
                        obj->transformations.rotation.z = quatVals[3];
                        obj->transformations.rotation.normalize();
                    }
                }


                // SPECYFICZNE PARAMETRY BEZIERA
                if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj)) {
                    ImGui::Checkbox("Pokaz lamana", &b->showPolygon);

                    if (ImGui::Button("Dodaj nowe punkty")) {
                        magicMode = true;
                        magicCurve = b;
                    }

                    ImGui::Text("Punkty kontrolne:");
                    // Iteracja ze sprytnym usuwaniem węzła (usun z krzywej)
                    for (auto it = b->points.begin(); it != b->points.end(); ) {
                        if (auto ptr = it->lock()) {
                            ImGui::Text(" - %s", ptr->name.c_str());
                            ImGui::SameLine();
                            ImGui::PushID(ptr.get());
                            if (ImGui::Button("Usun z krzywej")) {
                                it = b->points.erase(it);
                            } else {
                                ++it;
                            }
                            ImGui::PopID();
                        } else {
                            it = b->points.erase(it); // Usuwanie martwych wskaźników
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
                    {
                        t->UpdateBuffers();
                    }
                }
                else if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj))
                {
                    ImGui::Text("Geometria Punktu:");
                    ImGui::SliderFloat("Rozmiar", &p->size, 1.0f, 20.0f);
                }

                ImGui::Unindent();
                ImGui::Separator();
                ImGui::TreePop(); // Zamknięcie węzła "Edytuj"
            }
            ImGui::PopID();
        };











        if (ImGui::TreeNodeEx("Obiekty", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto& obj : sceneObjects)
            {
                // jezeli nie jest punktem
                if (!std::dynamic_pointer_cast<ScenePoint>(obj))
                {
                    renderObjectGuiRow(obj);
                }
            }
            ImGui::TreePop();
        }


        if (ImGui::TreeNodeEx("Punkty", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto& obj : sceneObjects)
            {
                if (std::dynamic_pointer_cast<ScenePoint>(obj))
                {
                    renderObjectGuiRow(obj);
                }
            }
            ImGui::TreePop();
        }

        // Zdejmujemy blokadę GUI tylko wtedy, gdy faktycznie weszliśmy w magiczny tryb
        // ZMIENIONY WARUNEK:
        if (isGuiDisabledThisFrame)
        {
            ImGui::EndDisabled();
        }
        ImGui::End();




        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Mat4 M_View = camera.getViewMatrix();
        Mat4 M_Proj  = createProjectionMatrix(fov, aspectRatio, min_camera_distance_view, max_camera_distance_view );

        Mat4 M_group_mouse(1.0f);
        Mat4 M_group_gui(1.0f);

        Quaternion previewDeltaQuat(1.0f, 0.0f, 0.0f, 0.0f);
        if (inputMode == INPUT_GUI)
        {
            if (guiRotMode == 0) previewDeltaQuat = Quaternion::fromAxisAngle(guiRotAxis[0], guiRotAxis[1], guiRotAxis[2], guiRotAngle * PI / 180.0f);
            else if (guiRotMode == 1) previewDeltaQuat = Quaternion(guiRotQuat[0], guiRotQuat[1], guiRotQuat[2], guiRotQuat[3]);
            previewDeltaQuat.normalize();
        }

        bool applyMousePreview = (inputMode == INPUT_MOUSE && isTransformationActive && (transformMode == COMMON_CENTER || transformMode == CURSOR_CENTER || transformMode == ENTIRE_SCENE));
        if (applyMousePreview)
        {
            Mat4 T_toOrigin = Mat4::translate_inverse(centerOfTransformations);
            Mat4 R_group = groupTransform.rotation.toMat4();
            Mat4 S_group = Mat4::scale(Vect3(groupTransform.scale, groupTransform.scale, groupTransform.scale));
            Mat4 T_toPos = Mat4::translate(centerOfTransformations + groupTransform.getPosition());
            M_group_mouse = T_toPos * R_group * S_group * T_toOrigin;
        }

        bool applyGuiPreviewGroup = (inputMode == INPUT_GUI && transformMode != LOCAL);
        if (applyGuiPreviewGroup)
        {
            Vect3 center = (transformMode == ENTIRE_SCENE) ? Vect3(0,0,0) : ((transformMode == CURSOR_CENTER) ? cursor.transform.getPosition() : centerOfSelection);
            Mat4 T_toOrigin = Mat4::translate_inverse(center);
            Mat4 R_group = previewDeltaQuat.toMat4();
            Mat4 S_group = Mat4::scale(Vect3(guiDeltaScale, guiDeltaScale, guiDeltaScale));
            Mat4 T_toPos = Mat4::translate(center + Vect3(guiDeltaPos[0], guiDeltaPos[1], guiDeltaPos[2]));
            M_group_gui = T_toPos * R_group * S_group * T_toOrigin;
        }



        // Wyciągamy stan transformacji do zmiennych, żeby przekazać je do krzywej:
        bool isTransforming = (inputMode == INPUT_MOUSE && isTransformationActive) || (inputMode == INPUT_GUI);
        bool isLocal = (transformMode == LOCAL);
        Vect3 localDelta = (inputMode == INPUT_MOUSE) ?
                           Vect3(groupTransform.posX, groupTransform.posY, groupTransform.posZ) :
                           Vect3(guiDeltaPos[0], guiDeltaPos[1], guiDeltaPos[2]);
        Mat4 activeGroupMat = (inputMode == INPUT_MOUSE) ? M_group_mouse : M_group_gui;



        // Rysowanie fizycznych obiektów z podglądem
        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, M_View.table);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, M_Proj.table);

        for (auto& obj : sceneObjects) {
            if (std::dynamic_pointer_cast<SceneBezierC0>(obj)) continue;

            bool isTargetGroup = false;
            bool isTargetLocal = false;

            if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj)) {
                if (p->isSelected || p->selectedCurvesCount > 0) {
                    isTargetGroup = (transformMode == ENTIRE_SCENE || transformMode == COMMON_CENTER || transformMode == CURSOR_CENTER);
                    isTargetLocal = (transformMode == LOCAL);
                }
            } else {
                if (obj->isSelected) {
                    isTargetGroup = (transformMode == ENTIRE_SCENE || transformMode == COMMON_CENTER || transformMode == CURSOR_CENTER);
                    isTargetLocal = (transformMode == LOCAL);
                }
            }
            if (transformMode == ENTIRE_SCENE) isTargetGroup = true;

            bool isMouseLocalTransform = (inputMode == INPUT_MOUSE && isTransformationActive && isTargetLocal);

            // Aplikacja i cofnięcie podglądu DLA SAMEGO RYSOWANIA (Stary, wydajny sposób)
            if ((inputMode == INPUT_GUI && isTargetLocal) || isMouseLocalTransform)
            {
                Quaternion oldRot = obj->transformations.rotation;
                float tPosX = isMouseLocalTransform ? groupTransform.posX : guiDeltaPos[0];
                float tPosY = isMouseLocalTransform ? groupTransform.posY : guiDeltaPos[1];
                float tPosZ = isMouseLocalTransform ? groupTransform.posZ : guiDeltaPos[2];
                float tScale = isMouseLocalTransform ? groupTransform.scale : guiDeltaScale;
                Quaternion tRot = isMouseLocalTransform ? groupTransform.rotation : previewDeltaQuat;

                obj->transformations.posX += tPosX;
                obj->transformations.posY += tPosY;
                obj->transformations.posZ += tPosZ;
                obj->transformations.scale *= tScale;
                obj->transformations.rotation = tRot * obj->transformations.rotation;
                obj->transformations.rotation.normalize();

                obj->Draw(shader);

                obj->transformations.posX -= tPosX;
                obj->transformations.posY -= tPosY;
                obj->transformations.posZ -= tPosZ;
                obj->transformations.scale /= tScale;
                obj->transformations.rotation = oldRot;
            }
            else if (applyGuiPreviewGroup && isTargetGroup) obj->Draw(shader, M_group_gui);
            else if (applyMousePreview && isTargetGroup) obj->Draw(shader, M_group_mouse);
            else obj->Draw(shader);
        }

        // Rysowanie krzywych Beziera (Z przekazaniem macierzy - TWÓJ POMYSŁ!)
        // Rysowanie krzywych Beziera (Z przekazaniem flagi transformAll na końcu)
        for (auto& obj : sceneObjects) {
            if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj)) {
                shader.use();
                b->DrawPolygon(shader, isTransforming, isLocal, localDelta, activeGroupMat, (transformMode == ENTIRE_SCENE));

                bezierShader.use();
                glUniformMatrix4fv(glGetUniformLocation(bezierShader.ID, "view"), 1, GL_FALSE, M_View.table);
                glUniformMatrix4fv(glGetUniformLocation(bezierShader.ID, "projection"), 1, GL_FALSE, M_Proj.table);

                b->DrawBezier(bezierShader, M_Proj * M_View, winWidth, winHeight, isTransforming, isLocal, localDelta, activeGroupMat, (transformMode == ENTIRE_SCENE));
            }
        }

        // Zresetowanie shadera i rysowanie narzędzi UI
        shader.use();

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

        // Skasuj obiekty (w tym puste krzywe)
        sceneObjects.erase(std::remove_if(sceneObjects.begin(), sceneObjects.end(),
                                          [](const std::shared_ptr<SceneObject>& o) { return o->pendingDelete; }), sceneObjects.end());
    }


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}