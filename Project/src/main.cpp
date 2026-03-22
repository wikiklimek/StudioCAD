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
#include "axis.h"

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


    int winWidth = 1024, winHeight = 768;

    float min_camera_distance_view = 0.1f;
    float max_camera_distance_view = 100.0f;

    float fov = PI / 4.0f;
    float aspectRatio = (float)winWidth/(float)winHeight;

    Vect3 cameraPos = Vect3(5.0f, -25.0f, 20.0f);
    Vect3 target = Vect3(0.0f, 0.0f, 0.0f);
    Vect3 up = Vect3(0.0f, 0.0f, 1.0f);



    float R = 3.0f;
    float r = 1.0f;
    int density_R = 30;
    int density_r = 15;
    bool buffersNeedUpdate = true;


    float objectColor[3] = { 1.0f, 1.0f, 0.0f };

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

    int max_density_R = 80;
    int min_density_R = 3;
    int max_density_r = 80;
    int min_density_r = 3;


    float max_rotations = PI;
    float min_rotations = -PI;
    float max_scale = 8.0f;
    float min_scale = 0.1f;
    float max_trans = 25.0f;
    float min_trans = -25.0f;




    bool drawTorus2 = false;



    std::vector<float> torusVertices;
    std::vector<unsigned int> torusIndices;

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);


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
            bool isClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

            if (isClick)
            {
                isDragging = true;

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
                        float angle = dy_world * 2.0f;
                        rotations[0] += angle;
                    }
                    else if (currentMode == ROTATE_Y)
                    {
                        float angle = dx_world * 2.0f;
                        rotations[1] += angle;
                    }
                    else if (currentMode == ROTATE_Z)
                    {
                        rotations[2] += dx_world * 2.0f;
                    }
                    else if (currentMode == SCALE)
                    {
                        float diff = (dx_world - dy_world) * 0.9f;
                        scale = std::max(0.01f, scale + diff);
                    }
                    else if (currentMode == ROTATE_FREE)
                    {
                        float sensitivity = 2.0f;

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

        ImGui::Separator();
        ImGui::Checkbox("Rysuj osie obrotow", &drawAxesEuler);
        ImGui::Checkbox("Rysuj drugi torus", &drawTorus2);

        ImGui::End();


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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        Mat4 M_View  = createViewMatrix(cameraPos, target, up);
        Mat4 M_Proj  = createProjectionMatrix(fov, aspectRatio, min_camera_distance_view, max_camera_distance_view );

        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, M_View.table);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, M_Proj.table);


        if(drawAxesEuler)
        {
            drawEulerAxes(shader, localAxisVAO, position, rotations, 100.0f);
        }

        // TORUS
        Mat4 M_Model = createModelMatrix(position, rotations, scale);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, M_Model.table);
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, objectColor);
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, torusIndices.size(), GL_UNSIGNED_INT, 0);


        // TORUS 2
        if (drawTorus2)
        {
            M_Model = createModelMatrix(position, rotations, scale)
                      * createModelMatrix(Vect3(-3.0f, 0.0f, 0.0f), Vect3(-1.0f, 0.0f, 0.0f), 1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, M_Model.table);
            glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, objectColor);
            glBindVertexArray(VAO);
            glDrawElements(GL_LINES, torusIndices.size(), GL_UNSIGNED_INT, 0);
        }



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