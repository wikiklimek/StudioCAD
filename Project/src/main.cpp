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
#include "previewFunctions.h"
#include "scenePoint.h"
#include "bakeTransform.h"
#include "camera.h"
#include "sceneBezierC0.h"
#include "guiManager.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


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



    Vect3 centerOfSelection(0.0f, 0.0f, 0.0f);





    auto firstTorus = std::make_shared<SceneTorus>("Torus Startowy", cursor.transform);
    firstTorus->Init();
    sceneObjects.push_back(firstTorus);


    int winWidth = 1024, winHeight = 768;



    float aspectRatio = (float)winWidth/(float)winHeight;

    GuiManager guiManager = GuiManager();
    Camera camera(Vect3(5.0f, -25.0f, 20.0f), Vect3(0.0f, 0.0f, 0.0f), Vect3(0.0f, 0.0f, 1.0f), PI / 4.0f);

    CameraDragMode camMode = CAM_NONE;
    bool isCamDragging = false;
    double startCamMouseX = 0, startCamMouseY = 0;


    bool isBoxSelecting = false;
    double boxStartX = 0, boxStartY = 0;
    double boxEndX = 0, boxEndY = 0;
    double boxSmallestXY = 5.0;

    bool isDragging = false;

    double lastMouseX = 0, lastMouseY = 0;
    double startMouseX = 0, startMouseY = 0;



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



    AppState appState;
    TransformManager tm;






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
                        Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, camera);
                        Vect3 intersection = getCursorIntersectionWithCameraPlane(rayDir, camera);

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

                // Kamera sama wie, jak przeliczyć te delty!
                camera.processMouseDrag(dx_world, dy_world, camMode);
            }

            // Obsługa klawiszy trybów i kliknięć
            if (isLeftClick)
            {
                if (magicMode)
                {
                    if (!isDragging) // Uruchomi się tylko raz na kliknięcie
                    {
                        // 1. Zaktualizuj pozycję kursora natychmiastowo
                        Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, camera);
                        Vect3 intersection = getCursorIntersectionWithCameraPlane(rayDir, camera);

                        cursor.transform.setPosition(intersection);
                        cursor.screenX = (float)mouseX;
                        cursor.screenY = (float)mouseY;

                        // 2. Stwórz punkt i dodaj do magicznej krzywej
                        auto p = std::make_shared<ScenePoint>(
                                "Punkt " + std::to_string(sceneObjects.size()+1),
                                cursor.transform);
                        p->Init();
                        sceneObjects.push_back(p);
                        magicCurve->points.push_back(p);
                    }
                    isDragging = true;
                }
                else if (!isDragging)
                {
                    if (appState.inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) appState.currentMode = TRANSLATE;
                    else if (appState.inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) appState.currentMode = ROTATE_FREE;
                    else if (appState.inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) appState.currentMode = ROTATE_X;
                    else if (appState.inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) appState.currentMode = ROTATE_Y;
                    else if (appState.inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) appState.currentMode = ROTATE_Z;
                    else if (appState.inputMode == INPUT_MOUSE && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) appState.currentMode = SCALE;
                    else
                    {
                        appState.currentMode = BOX;

                        // start rysowania ramki
                        isBoxSelecting = true;
                        boxStartX = mouseX;
                        boxStartY = mouseY;
                        boxEndX = mouseX;
                        boxEndY = mouseY;
                    }


                    if (appState.currentMode != BOX)
                    {
                        tm.startTransformation(centerOfSelection, cursor.transform.getPosition(), appState);
                        startMouseX = mouseX;
                        startMouseY = mouseY;
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

                    if (std::abs(boxEndX - boxStartX) < boxSmallestXY && std::abs(boxEndY - boxStartY) < boxSmallestXY)
                    {
                        handleSingleClickSelection(mouseX, mouseY, winWidth, winHeight, camera, sceneObjects);
                    }
                    else // select box rysowany ramką
                    {
                        performBoxSelection(boxStartX, boxStartY, boxEndX, boxEndY,
                                            winWidth, winHeight, camera, sceneObjects);
                    }
                }

                // Wypalanie transformacji lokalnych / grupowych
                tm.bakeMouseTransformations(sceneObjects, appState);


                isDragging = false;
                appState.currentMode = BOX;
            }

            // Aplikowanie myszki (Skróty T, R, S)
            if (isDragging && appState.currentMode != BOX)
            {
                float dx_screen = (float)(mouseX - startMouseX);
                float dy_screen = (float)(mouseY - startMouseY);

                float dx_world = (dx_screen / (float)winWidth) * 2.0f * aspectRatio;
                float dy_world = (dy_screen / (float)winHeight) * 2.0f;

                // CAŁY SWITCH/IF Z MYSZKĄ ZASTĄPIONY JEDNĄ FUNKCJĄ:
                tm.processMouseDrag(dx_world, dy_world, camera, appState);
            }


            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }

        // --- AKTUALIZACJA ZNACZNIKÓW PUNKTÓW ---
        for (auto& obj : sceneObjects)
        {
            if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj))
                p->selectedCurvesCount = 0;
        }

        for (auto& obj : sceneObjects)
        {
            if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj))
            {
                if (b->isSelected)
                {
                    for (auto& wp : b->points)
                    {
                        if (auto p = wp.lock())
                            p->selectedCurvesCount++;
                    }
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // WYWOŁANIE GUI ZAMKNIĘTE W JEDNEJ, CZYSTEJ LINIJCE!
        guiManager.Draw(sceneObjects, cursor, camera, appState,
                        isBoxSelecting, boxStartX, boxStartY, boxEndX, boxEndY,
                        magicMode, magicCurve, isCamDragging, centerOfSelection);



        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        Mat4 M_View = camera.getViewMatrix();
        Mat4 M_Proj = camera.getProjectionMatrix(aspectRatio);

        // ========================================================
        // 1. ZBUDOWANIE KONTEKSTU PODGLĄDU (OBIEKTY I KRZYWE)
        // ========================================================

        PreviewContext previewCtx = buildPreviewContext(appState, tm, guiManager, cursor.transform.getPosition(), centerOfSelection);


        // ========================================================
        // 2. RYSOWANIE FIZYCZNYCH OBIEKTÓW
        // ========================================================
        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, M_View.table);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, M_Proj.table);

        for (auto& obj : sceneObjects) {
            // Wywołujemy naszą zewnętrzną, czystą funkcję!
            drawObjectWithPreview(obj, shader, previewCtx);
        }

        // ========================================================
        // 3. RYSOWANIE KRZYWYCH BEZIERA (podajemy im ten sam kontekst!)
        // ========================================================
        for (auto& obj : sceneObjects)
        {
            if (auto b = std::dynamic_pointer_cast<SceneBezierC0>(obj))
            {
                shader.use();
                b->DrawPolygon(shader, previewCtx);

                bezierShader.use();
                glUniformMatrix4fv(glGetUniformLocation(bezierShader.ID, "view"), 1, GL_FALSE, M_View.table);
                glUniformMatrix4fv(glGetUniformLocation(bezierShader.ID, "projection"), 1, GL_FALSE, M_Proj.table);
                b->DrawBezier(bezierShader, M_Proj * M_View, winWidth, winHeight, previewCtx);
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