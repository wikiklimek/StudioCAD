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

    // Zmienne globalne GUI i logiki
    int transformMode = 0; // 0 = Lokalne, 1 = Wspólny Środek, 2 = Kursor, 3 - cala scena
    Vect3 centerOfSelection(0.0f, 0.0f, 0.0f);
    Transformations groupTransform; // Twoja pomocnicza struktura
    Vect3 currentPivot(0.0f, 0.0f, 0.0f); // Miejsce, wokół którego kręcimy (kursor lub środek)
    bool isTransformationActive = false; // Flaga określająca, czy jesteśmy w trakcie przeciągania

    // Dodanie domyślnego torusa na start [cite: 10]
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


    bool isDragging = false;
    DragMode currentMode = NONE;
    double lastMouseX = 0, lastMouseY = 0;


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

            // Ustawianie kursora i odznaczanie punktów [cite: 9, 19]
            if (isRightClick)
            {
                Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, cameraPos, target, up, fov);
                Vect3 intersection = getCursorIntersection(cameraPos, rayDir);
                cursor.transform.setPosition(intersection);
                cursor.screenX = (float)mouseX; cursor.screenY = (float)mouseY;
            }

            // Obsługa klawiszy trybów i kliknięć
            if (isLeftClick)
            {
                if (!isDragging) { // Wykonaj tylko raz przy kliknięciu (START)
                    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) currentMode = TRANSLATE;
                    else if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) currentMode = ROTATE_FREE;
                    else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) currentMode = ROTATE_X;
                    else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) currentMode = ROTATE_Y;
                    else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) currentMode = ROTATE_Z;
                    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) currentMode = SCALE;
                    else {
                        currentMode = NONE;

                        // ZAZNACZANIE OBIEKTÓW (Mouse Picking)
                        Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, cameraPos, target, up, fov);

                        // Opcjonalnie: trzymanie Shifta pozwala na zaznaczanie wielu obiektów bez odznaczania innych
                        bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                                            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

                        if (!shiftPressed) {
                            // Jeśli nie trzymamy Shifta, odznaczamy wszystkie
                            for (auto& obj : sceneObjects) obj->isSelected = false;
                        }

                        // Szukamy obiektu najbliżej promienia
                        float minDist = 10000.0f;
                        std::shared_ptr<SceneObject> closestObj = nullptr;

                        for (auto& obj : sceneObjects) {
                            Vect3 objPos = obj->transformations.getPosition();
                            // Wektor od kamery do obiektu
                            Vect3 toObj = objPos - cameraPos;
                            // Rzutujemy wektor na kierunek promienia
                            float projLength = Vect3::dot(toObj, rayDir);

                            // Sprawdzamy, czy obiekt jest przed kamerą
                            if (projLength > 0.0f) {
                                // Obliczamy odległość punktu od promienia (Twierdzenie Pitagorasa)
                                Vect3 projPoint = cameraPos + Vect3(rayDir.x * projLength, rayDir.y * projLength, rayDir.z * projLength);
                                float distToRay = std::sqrt(std::pow(objPos.x - projPoint.x, 2) +
                                                            std::pow(objPos.y - projPoint.y, 2) +
                                                            std::pow(objPos.z - projPoint.z, 2));

                                // Jeżeli odległość jest mniejsza niż jakiś próg (np. 1.5 dla torusa, możesz dobrać)
                                if (distToRay < 1.5f && projLength < minDist) {
                                    minDist = projLength;
                                    closestObj = obj;
                                }
                            }
                        }

                        // Przełączamy stan zaznaczenia trafionego obiektu
                        if (closestObj != nullptr) {
                            closestObj->isSelected = !closestObj->isSelected;
                        }
                    }

                    // Ustawiamy pivota na start transformacji
                    if (currentMode != NONE) {
                        isTransformationActive = true;
                        if (transformMode == 1) currentPivot = centerOfSelection;
                        else if (transformMode == 2) currentPivot = cursor.transform.getPosition();
                        else if (transformMode == 3) currentPivot = Vect3(0.0f, 0.0f, 0.0f); // Cała scena obraca się wokół (0,0,0)
                    }
                }
                isDragging = true;
            }
            else
            {
                // KONIEC KLIKNIĘCIA: Wypiekamy transformacje, jeśli jakieś były aktywne
                if (isTransformationActive) {
                    if (transformMode == 1 || transformMode == 2 || transformMode == 3) {
                        // Jeśli tryb to 3 (Cała Scena), używamy flagi applyToAll = true
                        bool applyToAll = (transformMode == 3);
                        bakeGroupTransform(sceneObjects, groupTransform, currentPivot, applyToAll);
                    }
                    isTransformationActive = false;
                }
                isDragging = false;
                currentMode = NONE;
            }

            // Transformacja zaznaczonych [cite: 22, 23, 24]
            // Transformacja obiektów (mysz przeciągana) [cite: 22, 23, 24, 34]
            if (isDragging && currentMode != NONE)
            {
                if (mouseX != lastMouseX || mouseY != lastMouseY)
                {
                    // 1. Obliczanie delty ruchu myszy
                    float dx_screen = (float)(mouseX - lastMouseX);
                    float dy_screen = (float)(mouseY - lastMouseY);

                    // Skalowanie ruchu myszy do przestrzeni świata (w oparciu o proporcje ekranu)
                    float dx_world = (dx_screen / (float)winWidth) * 2.0f * aspectRatio;
                    float dy_world = (dy_screen / (float)winHeight) * 2.0f;

                    // 2. Inicjalizacja pustych transformacji dla danej klatki
                    Vect3 deltaPos(0.0f, 0.0f, 0.0f);
                    float deltaScale = 1.0f;
                    Quaternion deltaQuat; // Domyślnie brak obrotu (w=1, x=0, y=0, z=0)

                    // 3. Wyznaczanie parametrów transformacji na podstawie wybranego trybu
                    if (currentMode == TRANSLATE)
                    {
                        deltaPos.x = 10.0f * dx_world;
                        deltaPos.y = -10.0f * dy_world; // Y rośnie w dół na ekranie, więc odwracamy
                    }
                    else if (currentMode == ROTATE_X)
                    {
                        // Oś X (1,0,0), kąt bierze się z ruchu góra-dół (dy_world)
                        deltaQuat = Quaternion::fromAxisAngle(1.0f, 0.0f, 0.0f, dy_world * 2.0f);
                    }
                    else if (currentMode == ROTATE_Y)
                    {
                        // Oś Y (0,1,0), kąt bierze się z ruchu lewo-prawo (dx_world)
                        deltaQuat = Quaternion::fromAxisAngle(0.0f, 1.0f, 0.0f, dx_world * 2.0f);
                    }
                    else if (currentMode == ROTATE_Z)
                    {
                        // Oś Z (0,0,1), rotacja w płaszczyźnie ekranu
                        deltaQuat = Quaternion::fromAxisAngle(0.0f, 0.0f, 1.0f, dx_world * 2.0f);
                    }
                    else if (currentMode == SCALE)
                    {
                        // Różnica w ruchu myszy wpływa na powiększenie/pomniejszenie
                        deltaScale = std::max(0.01f, 1.0f + (dx_world - dy_world) * 0.9f);
                    }
                    else if (currentMode == ROTATE_FREE)
                    {
                        // Obliczamy całkowitą długość przesunięcia myszy (to jest nasz kąt obrotu)
                        float angle = std::sqrt(dx_world * dx_world + dy_world * dy_world) * 2.0f;

                        if (angle > 0.0001f) {
                            // W swobodnym obrocie oś obrotu jest prostopadła do wektora przesunięcia myszy!
                            // Zatem ruch horyzontalny (dx_world) obraca wokół osi Y, a wertykalny (dy_world) wokół osi X.
                            deltaQuat = Quaternion::fromAxisAngle(dy_world, dx_world, 0.0f, angle);
                        }
                    }

                    // 4. Aplikacja wyliczonych transformacji do obiektów
                    if (transformMode == 0) // [0] TRYB LOKALNY: Transformacja względem środka każdego obiektu z osobna [cite: 22]
                    {
                        for (auto& obj : sceneObjects) {
                            if (!obj->isSelected) continue;

                            // Translacja
                            obj->transformations.posX += deltaPos.x;
                            obj->transformations.posY += deltaPos.y;
                            obj->transformations.posZ += deltaPos.z;

                            // Skala
                            obj->transformations.scale *= deltaScale;

                            // Obrót (CZYSTE KWATERNIONY!)
                            obj->transformations.rotation = deltaQuat * obj->transformations.rotation;
                            obj->transformations.rotation.normalize();
                        }
                    }
                    else // TRYBY GRUPOWE: [1] Wspólny Środek, [2] Kursor, [3] Cała Scena [cite: 23, 24, 34]
                    {
                        // W trybach grupowych na razie nie modyfikujemy samych obiektów.
                        // Zamiast tego akumulujemy zsumowane zmiany w pomocniczej strukturze grupy.
                        groupTransform.posX += deltaPos.x;
                        groupTransform.posY += deltaPos.y;
                        groupTransform.posZ += deltaPos.z;
                        groupTransform.scale *= deltaScale;

                        // Akumulacja obrotu
                        groupTransform.rotation = deltaQuat * groupTransform.rotation;
                        groupTransform.rotation.normalize();
                    }
                }
            }
            lastMouseX = mouseX; lastMouseY = mouseY;
        }


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ImGui::Begin("Sterowanie i Obiekty");

        // KURSOR INFO [cite: 8]
        ImGui::Text("Kursor 3D (Zmień PPM na scenie)");
        ImGui::DragFloat3("Pozycja (Scena)", &cursor.transform.posX, 0.1f);
        ImGui::Text("Pozycja (Ekran): X: %.1f, Y: %.1f", cursor.screenX, cursor.screenY);

        ImGui::Separator();
        // DODAWANIE OBIEKTÓW [cite: 10, 11, 12]
        if (ImGui::Button("Dodaj Torus"))
        {
            auto t = std::make_shared<SceneTorus>("Torus " + std::to_string(sceneObjects.size()+1), cursor.transform);
            t->Init(); sceneObjects.push_back(t);
        }
        ImGui::SameLine();
        if (ImGui::Button("Dodaj Punkt"))
        {
            auto p = std::make_shared<ScenePoint>("Punkt " + std::to_string(sceneObjects.size()+1), cursor.transform);
            p->Init(); sceneObjects.push_back(p);
        }
        ImGui::SameLine();
        if (ImGui::Button("Usuń Zaznacz."))
        { // [cite: 13]
            sceneObjects.erase(std::remove_if(sceneObjects.begin(), sceneObjects.end(),
                                              [](const std::shared_ptr<SceneObject>& o) { return o->isSelected; }), sceneObjects.end());
        }

        ImGui::Separator();
        // ŚRODEK I TRANSFORMACJE [cite: 21, 22, 23, 24]
        centerOfSelection = Vect3(0,0,0);
        int selCount = 0;
        for (auto& obj : sceneObjects)
        {
            if (obj->isSelected)
            {
                centerOfSelection = centerOfSelection + obj->transformations.getPosition();
                selCount++;
            }
        }

        if (selCount > 0)
            centerOfSelection = Vect3(centerOfSelection.x/selCount, centerOfSelection.y/selCount, centerOfSelection.z/selCount);

        ImGui::Text("Zaznaczonych: %d | Środek: %.1f, %.1f, %.1f", selCount, centerOfSelection.x, centerOfSelection.y, centerOfSelection.z);

        ImGui::RadioButton("Zaznacz. Lokalne", &transformMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Wspólny Środek", &transformMode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Względem Kursora", &transformMode, 2);
        ImGui::SameLine();
        ImGui::RadioButton("Cała Scena", &transformMode, 3);

        ImGui::Separator();
        // LISTA OBIEKTÓW [cite: 14, 15, 16, 18]
        ImGui::Text("Lista Obiektów na Scenie:");
        for (auto& obj : sceneObjects) {
            ImGui::PushID(obj.get());
            ImGui::Checkbox("##sel", &obj->isSelected); ImGui::SameLine();
            char nameBuf[128];
            strcpy(nameBuf, obj->name.c_str());
            if (ImGui::InputText("##name", nameBuf, IM_ARRAYSIZE(nameBuf))) obj->name = nameBuf;

            // Opcjonalna edycja parametrów dla torusa po zaznaczeniu na liście
            if (obj->isSelected)
            {
                if (auto t = std::dynamic_pointer_cast<SceneTorus>(obj))
                {
                    ImGui::Indent();
                    bool needsUpdate = false;
                    needsUpdate |= ImGui::SliderFloat("R", &t->R, min_R, max_R);
                    needsUpdate |= ImGui::SliderFloat("r", &t->r, min_r, max_r);
                    needsUpdate |= ImGui::SliderInt("density R", &t->density_R, min_density_R, max_density_R);
                    needsUpdate |= ImGui::SliderInt("density r", &t->density_r, min_density_r, max_density_r);
                    if (needsUpdate)
                        t->UpdateBuffers();
                    ImGui::Unindent();
                }
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



        // Obliczamy macierz grupy tylko raz na klatkę, jeśli transformacja trwa
        Mat4 M_group(1.0f);
        if (isTransformationActive && (transformMode == 1 || transformMode == 2 || transformMode == 3)) {
            Mat4 T_toOrigin = Mat4::translate_inverse(currentPivot);
            Mat4 R_group = groupTransform.rotation.toMat4();
            Mat4 S_group = Mat4::scale(Vect3(groupTransform.scale, groupTransform.scale, groupTransform.scale));
            Mat4 T_toPos = Mat4::translate(currentPivot + groupTransform.getPosition());
            M_group = T_toPos * R_group * S_group * T_toOrigin;
        }

        // Rysowanie wszystkich obiektów z listy
        for (auto& obj : sceneObjects) {
            // Decydujemy, czy na ten konkretny obiekt nałożyć grupową transformację
            bool applyGroupMatrix = isTransformationActive &&
                                    ((obj->isSelected && (transformMode == 1 || transformMode == 2)) || transformMode == 3);

            if (applyGroupMatrix) {
                obj->Draw(shader, M_group); // Otrzymuje tymczasowy obrót grupy (albo jest zaznaczony, albo tryb to Cała Scena)
            } else {
                obj->Draw(shader, Mat4(1.0f)); // Niezaznaczone rysują się normalnie
            }
        }

        // Rysowanie kursora [cite: 7]
        cursor.Draw(shader);

        // Opcjonalnie: rysowanie globalnych osi w środku świata lub na kursorze
        if(drawAxesEuler) {
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