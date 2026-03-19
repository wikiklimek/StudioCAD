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


#include "torus.h"
#include "matrixesModelViewProjection.h"
#include "grid.h"

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

    //4x antyaliasing
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

    // WAŻNE: Włączamy test głębokości (Depth testing)
    glEnable(GL_DEPTH_TEST);

    // TUTAJ DODAJESZ: Włączenie sprzętowego wygładzania krawędzi
    glEnable(GL_MULTISAMPLE);
    //glLineWidth(1.2f);

    // Tło sceny
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    Shader shader("src/shaders/torus.vs", "src/shaders/torus.fs");


    float min_camera_distance_view = 0.1f;
    float max_camera_distance_view = 100.0f;

    Vect3 cameraPos = Vect3(0.0f, -25.0f, 20.0f);
    Vect3 target = Vect3(0.0f, 0.0f, 0.0f);
    Vect3 up = Vect3(0.0f, 0.0f, 1.0f);


    // 1. Zmienne dla torusa i kamery
    float R = 3.0f; // Promień główny
    float r = 1.0f; // Promień przekroju
    int density_R = 30; // Gęstość siatki
    int density_r = 15; // Gęstość siatki
    bool buffersNeedUpdate = true; // Flaga przebudowy VBO/EBO


    float objectColor[3] = { 1.0f, 1.0f, 0.0f };
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    float shininessM = 1.1f;

    float position[3] = {0.0f, 0.0f, 5.0f};
    float scale  = 1.0f;
    float rotations[3]  = {0.0f, 0.0f, 0.0f};


    bool isDragging = false;
    DragMode currentMode = NONE;
    double lastMouseX = 0, lastMouseY = 0;


    float max_R = 10.0f;
    float min_R = 0.1f;
    float max_r = 5.0f;
    float min_r = 0.1f;

    int max_density_R = 60;
    int min_density_R = 3;
    int max_density_r = 60;
    int min_density_r = 3;


    float max_rotations = PI;
    float min_rotations = -PI;
    float max_scale = 8.0f;
    float min_scale = 0.1f;
    float max_trans = 25.0f;
    float min_trans = -25.0f;
    float min_m = 0.01f;
    float max_m = 4.0f;


    int winWidth = 1024, winHeight = 768;



    std::vector<float> torusVertices;
    std::vector<unsigned int> torusIndices;

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);


    std::vector<float> gridVertices;
    generateGrid(max_camera_distance_view, gridVertices);

    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


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

            //needsRedraw = true;
        }

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);



        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantCaptureMouse)
        {
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            bool isClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

            if (isClick)
            {
                isDragging = true;
                // Rozpoznawanie wciśniętego klawisza modyfikującego
                if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) currentMode = TRANSLATE;
                else if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) currentMode = ROTATE_FREE;
                else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) currentMode = ROTATE_X;
                else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) currentMode = ROTATE_Y;
                else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) currentMode = ROTATE_Z;
                else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) currentMode = SCALE;
                else currentMode = NONE;
            }

            if (!isClick)
            {
                isDragging = false;
                currentMode = NONE;
            }


            if (isDragging && currentMode != NONE)
                if (mouseX != lastMouseX || mouseY != lastMouseY)
                {
                    float dx_screen = (float) (mouseX - lastMouseX);
                    float dy_screen = (float) (mouseY - lastMouseY);


                    float viewScale = 1.0f;
                    float aspectRatio = (float) winWidth / (float) winHeight;

                    float dx_world = (dx_screen / (float) winWidth) * 2.0f * aspectRatio * viewScale;
                    float dy_world = (dy_screen / (float) winHeight) * 2.0f * viewScale;

                    if (currentMode == TRANSLATE)
                    {
                        float booster = 10.0f;

                        position[0] += booster * dx_world;
                        position[1] -= booster * dy_world;
                    }
                    else if (currentMode == ROTATE_X)
                    {
                        float angle = dy_world;
                        rotations[0] += angle;
                    }
                    else if (currentMode == ROTATE_Y)
                    {
                        float angle = dx_world;
                        rotations[1] += angle;
                    }
                    else if (currentMode == ROTATE_Z)
                    {
                        float x_ndc_old = (2.0f * (float) lastMouseX / (float) winWidth) - 1.0f;
                        float y_ndc_old = 1.0f - (2.0f * (float) lastMouseY / (float) winHeight);

                        float x_world_old = x_ndc_old * aspectRatio * viewScale;
                        float y_world_old = y_ndc_old * viewScale;

                        float x_ndc = (2.0f * (float) mouseX / (float) winWidth) - 1.0f;
                        float y_ndc = 1.0f - (2.0f * (float) mouseY / (float) winHeight);

                        float x_world = x_ndc * aspectRatio * viewScale;
                        float y_world = y_ndc * viewScale;


                        float rel_x_old = x_world_old - position[0];
                        float rel_y_old = y_world_old + position[1];

                        float rel_x = x_world - position[0];
                        float rel_y = y_world + position[1];


                        // wyznacznik - sinus
                        float det = rel_x_old * rel_y - rel_y_old * rel_x;
                        // iloczyn skalarny - cosinus
                        float dot = rel_x_old * rel_x + rel_y_old * rel_y;

                        float angle = std::atan2(det, dot);

                        rotations[2] -= angle;
                    }
                    else if (currentMode == SCALE)
                    {
                        float x_ndc_old = (2.0f * (float) lastMouseX / (float) winWidth) - 1.0f;
                        float y_ndc_old = 1.0f - (2.0f * (float) lastMouseY / (float) winHeight);

                        float x_world_old = x_ndc_old * aspectRatio * viewScale;
                        float y_world_old = y_ndc_old * viewScale;

                        float x_ndc = (2.0f * (float) mouseX / (float) winWidth) - 1.0f;
                        float y_ndc = 1.0f - (2.0f * (float) mouseY / (float) winHeight);

                        float x_world = x_ndc * aspectRatio * viewScale;
                        float y_world = y_ndc * viewScale;

                        float rel_x_old = x_world_old - position[0];
                        float rel_y_old = y_world_old + position[1];

                        float rel_x = x_world - position[0];
                        float rel_y = y_world + position[1];

                        float old_length = std::sqrt(rel_x_old * rel_x_old + rel_y_old * rel_y_old);
                        float length = std::sqrt(rel_x * rel_x + rel_y * rel_y);

                        float diff = length - old_length;
                        scale = std::max(0.01f, scale + diff);
                    }
                    else if (currentMode == ROTATE_FREE)
                    {
                        float sensitivity = 1.8f;

                        float angleX = dy_world * sensitivity;
                        rotations[0] += angleX;

                        float angleY = +dx_world * sensitivity;
                        rotations[1] += angleY;
                    }


                    for (int k = 0; k < 3; k++)
                    {
                        while (rotations[k] > max_rotations)
                            rotations[k] -= 2 * PI;

                        while (rotations[k] < min_rotations)
                            rotations[k] += 2 * PI;
                    }

                    scale = fmax(min_scale, fmin(max_scale, scale));

                    position[0] = fmax(min_trans, fmin(max_trans, position[0]));
                    position[1] = fmax(min_trans, fmin(max_trans, position[1]));
                    position[2] = fmax(min_trans, fmin(max_trans, position[2]));
                }


            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ImGui::Begin("Sterowanie");
        ImGui::Text("Kliknij + przeciagnij:");
        ImGui::BulletText("[T] - Przesuniecie");
        ImGui::BulletText("[X], [Y], [Z] - Obrot wokół osi");
        ImGui::BulletText("[S] - Skalowanie");
        ImGui::Separator();

        if (ImGui::SliderFloat("Promień gł. (R)", &R, min_R, max_R)) buffersNeedUpdate = true;
        if (ImGui::SliderFloat("Promień przekr. (r)", &r, min_r, max_r)) buffersNeedUpdate = true;
        if (ImGui::SliderInt("Gęstość R", &density_R, min_density_R, max_density_R)) buffersNeedUpdate = true;
        if (ImGui::SliderInt("Gęstość r", &density_r, min_density_r, max_density_r)) buffersNeedUpdate = true;

        ImGui::DragFloat3("Pozycja (XYZ)", position, 0.1f, min_trans, max_trans);//) imguiChanged = true;
        ImGui::DragFloat("Skala", &scale, 0.05f, min_scale, max_scale);//) imguiChanged = true;
        ImGui::DragFloat3("Obrót (XYZ)", rotations, 0.05f, min_rotations, max_rotations);//) imguiChanged = true;
        ImGui::Separator();


        ImGui::ColorEdit3("Kolor modelu", objectColor);


        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();



        // 3. Przebudowa buforów (tylko gdy zmieniły się parametry torusa)
        if (buffersNeedUpdate)
        {
            torusVertices.clear();
            torusIndices.clear();
            generateTorus(R, r, density_R, density_r, torusVertices, torusIndices);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, torusVertices.size() * sizeof(float), torusVertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,torusIndices.size() * sizeof(unsigned int), torusIndices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);


            buffersNeedUpdate = false;
        }


        //if (needsRedraw)
        //{
            // 4. Renderowanie
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Pamiętaj o włączeniu i czyszczeniu GL_DEPTH_TEST!

            shader.use();


        //Mat4 M_View  = createViewMatrix(Vect3(0.0f, 0.0f, 15.0f), Vect3(0.0f, 0.0f, 0.0f), Vect3(0.0f, 1.0f, 0.0f));
        // Kamera jest wycofana na osi Y (-15) i podniesiona na osi Z (10)
        // Patrzy na środek (0,0,0), a wektorem "góry" jest teraz oś Z (0,0,1)
        Mat4 M_View  = createViewMatrix(cameraPos, target, up);
        Mat4 M_Proj  = createProjectionMatrix(PI / 4.0f, (float)winWidth/winHeight, min_camera_distance_view, max_camera_distance_view );

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, M_View.table);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, M_Proj.table);


        // TORUS

        // Macierz modelu dla torusa (zmienia się suwakami/myszą)
        Mat4 M_Model = createModelMatrix(position, rotations, scale);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, M_Model.table);

        // Wysyłanie właściwego koloru torusa
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, objectColor);

        // Rysowanie indeksowane torusa (Twój stary kod)
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, torusIndices.size(), GL_UNSIGNED_INT, 0); // Zmieniono na GL_LINES!


        // TORUS 2
        M_Model = createModelMatrix( position, rotations, scale)
                * createModelMatrix(Vect3(-3.0f, 0.0f, 0.0f),Vect3(-1.0f, 0.0f, 0.0f), 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, M_Model.table);
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, objectColor);
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, torusIndices.size(), GL_UNSIGNED_INT, 0); // Zmieniono na GL_LINES!



        // --- Rysowanie ImGui na końcu ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}