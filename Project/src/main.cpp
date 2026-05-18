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
#include "sceneBezierC2.h"
#include "sceneSplineInterpolating.h"
#include "selectionManager.h"
#include "sceneSurface.h"
#include "deleteManager.h"

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
    glLineWidth(1.2f);


    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    Shader shader("src/shaders/torus.vs", "src/shaders/torus.fs");

    Shader bezierLineStripShader("src/shaders/bezier_line_strip.vs", "src/shaders/bezier_line_strip.fs");
    Shader bezierGeomShader("src/shaders/bezier_geom.vs", "src/shaders/bezier_geom.fs", "src/shaders/bezier_geom.gs");

    Shader bsplineLineStripShader("src/shaders/bspline_line_strip.vs", "src/shaders/bezier_line_strip.fs");
    Shader bsplineGeomShader("src/shaders/bezier_geom.vs", "src/shaders/bezier_geom.fs", "src/shaders/bspline_geom.gs");

    Shader splineAlgebraicInterpolationShader("src/shaders/bezier_geom.vs", "src/shaders/bezier_geom.fs", "src/shaders/spline_algebraic_geom.gs");

    Shader* surfaceShaderC0 = new Shader("src/shaders/surface.vs", "src/shaders/surface.fs", "src/shaders/surface.tcs", "src/shaders/surfaceC0.tes");
    Shader* surfaceShaderC2 = new Shader("src/shaders/surface.vs", "src/shaders/surface.fs", "src/shaders/surface.tcs", "src/shaders/surfaceC2.tes");

    BezierDrawMode currentBezierDrawMode = GEOMETRY;
    Shader * bezierShader = currentBezierDrawMode == GEOMETRY ?  &bezierGeomShader : &bezierLineStripShader;
    Shader * bsplineShader = currentBezierDrawMode == GEOMETRY ? &bsplineGeomShader : &bsplineLineStripShader;
    Shader * interpolatingShader = &splineAlgebraicInterpolationShader; //domyslnie geometry shader

    bool magicMode = false;
    std::shared_ptr<SceneBezier> magicCurve = nullptr;


    Cursor cursor;
    cursor.Init();

    Axis sceneAxis;
    sceneAxis.Init();

    AppState appState;
    TransformManager tm;


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

                // Kamera sama sie przesuwa
                camera.processMouseDrag(dx_world, dy_world, camMode);
            }


            if (isLeftClick)
            {
                if (magicMode)
                {
                    if (!isDragging) // Uruchomi się tylko raz na kliknięcie
                    {
                        // kursor
                        Vect3 rayDir = getRayDirection(mouseX, mouseY, winWidth, winHeight, camera);
                        Vect3 intersection = getCursorIntersectionWithCameraPlane(rayDir, camera);

                        cursor.transform.setPosition(intersection);
                        cursor.screenX = (float)mouseX;
                        cursor.screenY = (float)mouseY;

                        // nowy ounkt
                        auto p = std::make_shared<ScenePoint>(
                                "Punkt " + std::to_string(sceneObjects.size()+1),
                                cursor.transform);
                        p->Init();
                        p->globalCurvesCount++;
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


                //dzieje sie viagle przy kliknietym lewym
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

                    tm.wasSelectionChanged = true;


                    // dla klikania/select boxa zawsze stare obiekty pzrestaja byc zaczone
                    // ile krzywych nalezacych do punktu jest zaznaczonych - zerowanie
                    unselectObjectsAndVirtualPointsAndCleanPointsSelectedBeziers(sceneObjects);


                    if (std::abs(boxEndX - boxStartX) < boxSmallestXY && std::abs(boxEndY - boxStartY) < boxSmallestXY)
                    {
                        //od razu wyznaczamy punkty de bora ktore beda przesuwane
                        std::shared_ptr<SceneBezierC2> selectedVirtualBezierOwner;
                        if(handleSingleClickSelection(mouseX, mouseY, winWidth, winHeight, camera,
                                                      sceneObjects, selectedVirtualBezierOwner))
                        {
                            selectedVirtualBezierOwner->markAffectedDeBoorPoints();
                        }

                    }
                    else // select box rysowany ramką
                    {
                        performBoxSelection(boxStartX, boxStartY, boxEndX, boxEndY,
                                            winWidth, winHeight, camera, sceneObjects);
                    }
                }


                // Wypalanie transformacji
                if(isDragging)
                {
                    tm.bakeMouseTransformations(sceneObjects, appState);

                    isDragging = false;
                    appState.currentMode = BOX;
                }



            }


            // transformacje myszka
            if (isDragging && appState.currentMode != BOX)
            {
                float dx_screen = (float)(mouseX - startMouseX);
                float dy_screen = (float)(mouseY - startMouseY);

                float dx_world = (dx_screen / (float)winWidth) * 2.0f * aspectRatio;
                float dy_world = (dy_screen / (float)winHeight) * 2.0f;


                tm.processMouseDrag(dx_world, dy_world, camera, appState);
            }


            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        guiManager.Draw(sceneObjects, cursor, camera, appState,
                        isBoxSelecting, boxStartX, boxStartY, boxEndX, boxEndY,
                        magicMode, magicCurve, isCamDragging, centerOfSelection);



        // JEZELI JEST ZANZCINY JAKIS PUNKT WIRTUALNY, TO TYLKO ON JEST ZAZNCZINY
        // bo punktów wirtualnych nie ma na liscie gui!
        if (guiManager.wasSelectionChanged)
        {
            unselectVirtualPointsAndActualizePointsSelectedBeziers(sceneObjects);
        }



        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);






        Mat4 Base_View = camera.getViewMatrix();
        Mat4 Base_Proj = camera.getProjectionMatrix(aspectRatio);

        PreviewContext previewCtx = buildPreviewContext(appState, tm, guiManager, cursor.transform.getPosition(), centerOfSelection);

        // rysowanie całej sceny
        auto RenderScenePass = [&](Mat4 V, Mat4 P, bool stereoscopy, Vect3 sColor)
        {

            auto updateShader = [&](Shader* s) {
                s->use();
                glUniformMatrix4fv(glGetUniformLocation(s->ID, "view"), 1, GL_FALSE, V.table);
                glUniformMatrix4fv(glGetUniformLocation(s->ID, "projection"), 1, GL_FALSE, P.table);
                glUniform1i(glGetUniformLocation(s->ID, "isStereo"), stereoscopy ? 1 : 0);
                glUniform3f(glGetUniformLocation(s->ID, "stereoColor"), sColor.x, sColor.y, sColor.z);
            };

            updateShader(&shader);
            updateShader(bezierShader);
            updateShader(bsplineShader);
            updateShader(interpolatingShader);

            updateShader(surfaceShaderC0);
            updateShader(surfaceShaderC2);

            shader.use();
            for (auto& obj : sceneObjects)
            {
                drawObjectWithPreview(obj, shader, previewCtx);
            }

            for (auto& obj : sceneObjects)
            {
                if (obj->objectType == ObjectType::BezierCurveC0)
                {
                    bezierShader->use();
                    auto b = std::static_pointer_cast<SceneBezierC0>(obj);
                    b->DrawBezier(*bezierShader, P * V, winWidth, winHeight, previewCtx, currentBezierDrawMode);
                }
                else if (obj->objectType == ObjectType::BezierCurveC2)
                {
                    auto b2 = std::static_pointer_cast<SceneBezierC2>(obj);
                    Shader* activeShader = (b2->currentBasis == BezierBasisMode::B_SPLINE) ? bsplineShader : bezierShader;
                    activeShader->use();
                    b2->DrawBezier(*activeShader, P * V, winWidth, winHeight, previewCtx, currentBezierDrawMode);
                }
                else if (obj->objectType == ObjectType::SplineInterpolating)
                {
                    auto s = std::static_pointer_cast<SceneSplineInterpolating>(obj);
                    Shader* activeShader = (s->currentBasis == InterpolationBasisMode::ALGEBRAIC) ? interpolatingShader : bezierShader;
                    activeShader->use();
                    s->DrawBezier(*activeShader, P * V, winWidth, winHeight, previewCtx, currentBezierDrawMode);
                }
                else if (obj->objectType == ObjectType::BezierSurfaceC0)
                {
                    auto s = std::static_pointer_cast<SceneSurfaceC0>(obj);
                    s->DrawSurface(*surfaceShaderC0, previewCtx);
                }
                else if (obj->objectType == ObjectType::BezierSurfaceC2)
                {
                    auto s = std::static_pointer_cast<SceneSurfaceC2>(obj);
                    s->DrawSurface(*surfaceShaderC2, previewCtx);
                }
            }

            shader.use();
            for (auto& obj : sceneObjects)
            {
                if (obj->objectType == ObjectType::BezierCurveC0 ||
                    obj->objectType == ObjectType::BezierCurveC2 ||
                    obj->objectType == ObjectType::SplineInterpolating ||
                    obj->objectType == ObjectType::BezierSurfaceC0 ||
                    obj->objectType == ObjectType::BezierSurfaceC2)
                {
                    std::shared_ptr<ScenePolygon> poly = std::dynamic_pointer_cast<ScenePolygon>(obj);

                    // Jeśli rzutowanie się powiodło (czyli obiekt dziedziczy po ScenePolygon), to tu bedzie git
                    if (poly)
                    {
                        poly->DrawPolygon(shader, previewCtx);
                    }

                }
            }

            // ta płaszczyzna preview
            if (guiManager.isNewSurfacePanelOpen && guiManager.previewSurface)
            {
                PreviewContext dummyCtx;
                auto ps = guiManager.previewSurface;

                if (ps->objectType == ObjectType::BezierSurfaceC0)
                    ps->DrawSurface(*surfaceShaderC0, dummyCtx);
                else
                    ps->DrawSurface(*surfaceShaderC2, dummyCtx);

                ps->DrawPolygon(shader, dummyCtx);


                shader.use();
                for (auto& p : guiManager.previewPoints)
                {
                    p->Draw(shader);
                }
            }

            shader.use();
            cursor.Draw(shader);
            sceneAxis.Draw(shader, Vect3(0.0f, 0.0f, 0.0f), Vect3(0.0f, 0.0f, 0.0f), 10000.0f);

        };


        // stereoskopia
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!guiManager.isStereoMode)
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            RenderScenePass(Base_View, Base_Proj, false, Vect3(0,0,0));
        }
        else
        {
            // lewe oko - czerwone
            Mat4 P_Left(0, 0, 0, 0), V_LeftShift(0, 0, 0, 0);
            getStereoMatrices(camera.fov, aspectRatio, camera.nearPlane, camera.farPlane, guiManager.eyeSeparation, guiManager.focalDistance, true, P_Left, V_LeftShift);

            glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
            RenderScenePass(V_LeftShift * Base_View, P_Left, true, Vect3(1.0f, 0.0f, 0.0f));

            // prawe oko - niebieski
            glClear(GL_DEPTH_BUFFER_BIT); // clean depth, but colours leave alone where they are

            Mat4 P_Right(0, 0, 0, 0), V_RightShift(0, 0, 0, 0);
            getStereoMatrices(camera.fov, aspectRatio, camera.nearPlane, camera.farPlane, guiManager.eyeSeparation, guiManager.focalDistance, false, P_Right, V_RightShift);

            glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
            RenderScenePass(V_RightShift * Base_View, P_Right, true, Vect3(0.0f, 0.0f, 1.0f));

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }



        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);


        //Potezny Mechanizm Usuwania Obiektów
        deleteObjects(guiManager, sceneObjects);

        // Resetowanie jednorazowaych flag
        for (auto& obj : sceneObjects)
        {
            obj->wasGuiSelectionChanged = false;
            if (obj->objectType == ObjectType::Point)
            {
                auto p = std::static_pointer_cast<ScenePoint>(obj);
                p->wasGuiEdited = false;
            }
        }

        tm.wasSelectionChanged = false;
        tm.wasBaked = false;
        guiManager.wasSelectionChanged = false;
        guiManager.wasBaked = false;
        guiManager.deleteSelectedPressed = false;
    }


    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}