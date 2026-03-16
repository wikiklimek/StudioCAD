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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


void renderCPU(std::vector<unsigned char>& buffer, int width, int height, int pixelSize,
               Vect3 objectColor, Vect3 lightColor, float m, const Mat4& D_M_prime)
{
    Mat4 D_M_prime_sym = D_M_prime + D_M_prime.transpose();

    Vect3 bgColor(0.05f, 0.05f, 0.05f);

    float p00 = D_M_prime.table[0]; float p10 = D_M_prime.table[1]; float p20 = D_M_prime.table[2]; float p30 = D_M_prime.table[3];
    float p01 = D_M_prime.table[4]; float p11 = D_M_prime.table[5]; float p21 = D_M_prime.table[6]; float p31 = D_M_prime.table[7];
    float p02 = D_M_prime.table[8]; float p12 = D_M_prime.table[9]; float p22 = D_M_prime.table[10];float p32 = D_M_prime.table[11];
    float p03 = D_M_prime.table[12];float p13 = D_M_prime.table[13];float p23 = D_M_prime.table[14];float p33 = D_M_prime.table[15];

    float viewScale = 1.0f;
    float aspectRatio = (float)width / (float)height;

    //Vect3 viewDir(0.0f, 0.0f, 1.0f);
    Vect3 cameraPosition(0.0f, 0.0f, 6.0f);

    for (int y = 0; y < height; y += pixelSize)
    {
        for (int x = 0; x < width; x += pixelSize)
        {
            float px = x + pixelSize * 0.5f;
            float py = height - 1 - (y + pixelSize * 0.5f);

            float rayX = ((px / (float)width) * 2.0f - 1.0f) * aspectRatio * viewScale;
            float rayY = ((py / (float)height) * 2.0f - 1.0f) * viewScale;

            float A = p22;
            float B = (p02 + p20) * rayX + (p12 + p21) * rayY + (p23 + p32);
            float C = p00 * rayX * rayX + p11 * rayY * rayY + p33 +
                      (p01 + p10) * rayX * rayY + (p03 + p30) * rayX + (p13 + p31) * rayY;

            float delta = B * B - 4.0f * A * C;
            Vect3 finalColor = bgColor;

            if (delta >= 0.0f)
            {
                float sqrtDelta = std::sqrt(delta);
                float z1 = (-B - sqrtDelta) / (2.0f * A);
                float z2 = (-B + sqrtDelta) / (2.0f * A);
                float hitZ = std::max(z1, z2);

                Vect3 hitPointWorld(rayX, rayY, hitZ);
                Vect4 hitPointWorld4(hitPointWorld.x, hitPointWorld.y, hitPointWorld.z, 1.0f);

                // normalna - gradient
                Vect4 normal4 = D_M_prime_sym * hitPointWorld4;
                Vect3 worldNormal = Vect3(normal4.x, normal4.y, normal4.z).normalize();


                //float w_dot_n = std::max(Vect3::dot(viewDir, worldNormal), 0.0f);

                Vect3 toObservator = (cameraPosition - hitPointWorld).normalize();
                float w_dot_n = std::max(Vect3::dot(toObservator, worldNormal), 0.0f);
                float specularTerm = std::pow(w_dot_n, m);

                finalColor = (lightColor * specularTerm) * objectColor;
            }

            unsigned char cr = (unsigned char)(std::min(finalColor.x, 1.0f) * 255.0f);
            unsigned char cg = (unsigned char)(std::min(finalColor.y, 1.0f) * 255.0f);
            unsigned char cb = (unsigned char)(std::min(finalColor.z, 1.0f) * 255.0f);


            for (int by = 0; by < pixelSize; ++by)
            {
                for (int bx = 0; bx < pixelSize; ++bx)
                {
                    if (x + bx < width && y + by < height) // zeby nie wyjechac poza obraz
                    {
                        int index = ((y + by) * width + (x + bx)) * 3;
                        buffer[index + 0] = cr;
                        buffer[index + 1] = cg;
                        buffer[index + 2] = cb;
                    }
                }
            }
        }
    }
}


enum DragMode { NONE, TRANSLATE, ROTATE_X, ROTATE_Y, ROTATE_Z, SCALE, ROTATE_FREE };

int main()
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Ellipsoida", NULL, NULL);
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

    Shader shader("src/shaders/raycasting_cpu.vs", "src/shaders/raycasting_cpu.fs");


    float vertices[] = {
            1.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f };

    unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3 };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int winWidth = 1024, winHeight = 768;
    std::vector<unsigned char> frameBuffer(winWidth * winHeight * 3, 0);

    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, winWidth, winHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");


    float objectColor[3] = { 1.0f, 1.0f, 0.0f };
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    float paramA = 3.0f, paramB = 5.0f, paramC = 1.0f;
    float shininessM = 1.1f;

    float position[3] = {0.0f, 0.0f, 0.0f};
    float scale  = 1.0f;
    float rotations[3]    = {0.0f, 0.0f, 0.0f};


    float lastParamA = paramA, lastParamB = paramB, lastParamC = paramC;
    float lastPos[3] = {position[0], position[1], position[2]};
    float lastRot[3] = {rotations[0], rotations[1], rotations[2]};
    float lastScale = scale;
    float lastM = shininessM;


    int currentPixelSize = 1;
    double lastFrameTime = 0.0; // sekundy - ile trawło rysowanie poprzedbiej klatki

    bool isDragging = false;
    DragMode currentMode = NONE;
    double lastMouseX = 0, lastMouseY = 0;


    float max_rotations = PI;
    float min_rotations = -PI;
    float max_scale = 8.0f;
    float min_scale = 0.1f;
    float max_trans = 4.0f;
    float min_trans = -4.0f;
    float min_a = 0.1f;
    float max_a = 20.0f;
    float min_m = 0.01f;
    float max_m = 4.0f;

    float range_trans = max_trans - min_trans;
    float range_rot = max_rotations - min_rotations;
    float range_scale = max_scale - min_scale;
    float range_a = max_a - min_a;
    float range_m = max_m - min_m;


    bool needsRedraw = true;
    bool wasInteracting = false;
    bool changesLightColor = false;

    while (!glfwWindowShouldClose(window))
    {
        // rozmiar okna
        int currentW, currentH;
        glfwGetFramebufferSize(window, &currentW, &currentH);
        if (currentW > 0 && currentH > 0 && (currentW != winWidth || currentH != winHeight))
        {
            winWidth = currentW;
            winHeight = currentH;
            frameBuffer.resize(winWidth * winHeight * 3);
            glBindTexture(GL_TEXTURE_2D, screenTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, winWidth, winHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

            needsRedraw = true;
        }

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);



        ImGuiIO& io = ImGui::GetIO();
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        bool isClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        if (isClick)
        {
                // if (checkHit((float)mouseX, (float)mouseY, winWidth, winHeight, D_M_prime))
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
            if(mouseX != lastMouseX || mouseY != lastMouseY)
        {
            float dx_screen = (float)(mouseX - lastMouseX);
            float dy_screen = (float)(mouseY - lastMouseY);

            //currentSpeed = std::sqrt(dx_screen * dx_screen + dy_screen * dy_screen);

            float viewScale = 1.0f;
            float aspectRatio = (float)winWidth / (float)winHeight;

            float dx_world = (dx_screen / (float)winWidth) * 2.0f * aspectRatio * viewScale;
            float dy_world = (dy_screen / (float)winHeight) * 2.0f * viewScale;

            if (currentMode == TRANSLATE)
            {
                position[0] += dx_world;
                position[1] += dy_world;
            }
            else if (currentMode == ROTATE_X )
            {
                float angle = -dy_world;
                rotations[0] += angle;
            }
            else if ( currentMode == ROTATE_Y )
            {
                float angle = dx_world;
                rotations[1] += angle;
            }
            else if (currentMode == ROTATE_Z)
            {

                    float x_ndc_old = (2.0f * (float)lastMouseX / (float)winWidth) - 1.0f;
                    float y_ndc_old = 1.0f - (2.0f * (float)lastMouseY / (float)winHeight);

                    float x_world_old = x_ndc_old * aspectRatio * viewScale;
                    float y_world_old = y_ndc_old * viewScale;

                    float x_ndc = (2.0f * (float)mouseX / (float)winWidth) - 1.0f;
                    float y_ndc = 1.0f - (2.0f * (float)mouseY / (float)winHeight);

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
                float x_ndc_old = (2.0f * (float)lastMouseX / (float)winWidth) - 1.0f;
                float y_ndc_old = 1.0f - (2.0f * (float)lastMouseY / (float)winHeight);

                float x_world_old = x_ndc_old * aspectRatio * viewScale;
                float y_world_old = y_ndc_old * viewScale;

                float x_ndc = (2.0f * (float)mouseX / (float)winWidth) - 1.0f;
                float y_ndc = 1.0f - (2.0f * (float)mouseY / (float)winHeight);

                float x_world = x_ndc * aspectRatio * viewScale;
                float y_world = y_ndc * viewScale;

                float rel_x_old = x_world_old - position[0];
                float rel_y_old = y_world_old + position[1];

                float rel_x = x_world - position[0];
                float rel_y = y_world + position[1];

                float old_length = std::sqrt(rel_x_old * rel_x_old + rel_y_old * rel_y_old);
                float length     = std::sqrt(rel_x * rel_x + rel_y * rel_y);

                float diff = length - old_length;
                scale = std::max(0.01f, scale + diff);
            }
            else if (currentMode == ROTATE_FREE)
            {
                float sensitivity = 1.0f;

                float angleX = dy_world * sensitivity;
                rotations[0] -= angleX;

                float angleY = dx_world * sensitivity;
                rotations[1] += angleY;
            }

            //rotations[0] = fmax(min_rotations, fmin(max_rotations, rotations[0]));
            //rotations[1] = fmax(min_rotations, fmin(max_rotations, rotations[1]));
            //rotations[2] = fmax(min_rotations, fmin(max_rotations, rotations[2]));
            for(int k = 0; k < 3; k++)
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


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool imguiChanged = false; // Flaga sprawdzająca ruch suwakami

        ImGui::Begin("Sterowanie");
        ImGui::Text("Kliknij + przeciagnij:");
        ImGui::BulletText("[T] - Przesuniecie");
        ImGui::BulletText("[X], [Y], [Z] - Obrot wokół osi");
        ImGui::BulletText("[S] - Skalowanie");
        ImGui::Separator();

        if (ImGui::DragFloat3("Pozycja (XYZ)", position, 0.05f, min_trans, max_trans)) imguiChanged = true;
        if (ImGui::DragFloat("Skala", &scale, 0.05f, min_scale, max_scale)) imguiChanged = true;
        if (ImGui::DragFloat3("Obrót (XYZ)", rotations, 0.05f, min_rotations, max_rotations)) imguiChanged = true;
        ImGui::Separator();

        if (ImGui::DragFloat("Parametr a", &paramA, 0.01f, min_a, max_a)) imguiChanged = true;
        if (ImGui::DragFloat("Parametr b", &paramB, 0.01f, min_a, max_a)) imguiChanged = true;
        if (ImGui::DragFloat("Parametr c", &paramC, 0.01f, min_a, max_a)) imguiChanged = true;
        ImGui::Separator();


        if (ImGui::ColorEdit3("Kolor swiatla", lightColor)) {imguiChanged = true; changesLightColor = true;}
        if (ImGui::SliderFloat("Potega (m)", &shininessM, min_m, max_m)) imguiChanged = true;


        //ImGui::Text("Rozmiar piksela: %d", currentPixelSize);
        //ImGui::Text("Czas renderu CPU: %.3f ms", lastFrameTime * 1000.0);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();


        float dM = std::abs(shininessM - lastM) / range_m;
        float dPos = (std::abs(position[0] - lastPos[0]) + std::abs(position[1] - lastPos[1]) + std::abs(position[2] - lastPos[2])) / range_trans;
        float dScale = std::abs(scale - lastScale) / range_scale;
        float dParams = (std::abs(paramA - lastParamA) + std::abs(paramB - lastParamB) + std::abs(paramC - lastParamC)) / (range_a);

        float dRotXYZ[3] =
                {std::abs(rotations[0] - lastRot[0]),
                 std::abs(rotations[1] - lastRot[1]),
                 std::abs(rotations[2] - lastRot[2])};

        float dRot = 0.0f;
        for(int k = 0; k < 3; k++)
        {
            while (dRotXYZ[k] >= 1.5f * PI)
                dRotXYZ[k] = fmax(0.0f, dRotXYZ[k] - 2 * PI);

            dRot += dRotXYZ[k];
        }
        dRot /= range_rot;


        float totalChange = dM + dPos + dRot + dScale + dParams;

        bool isInteracting = (totalChange > 0.0001f);
        //bool isInteracting = (isDragging && currentMode != NONE) || imguiChanged;

        if (isInteracting)
        {
            float sensitivity = 100.0f;

            currentPixelSize = std::clamp((int)(totalChange * sensitivity) + 2, 2, 32);
            needsRedraw = true;
        }
        else if (wasInteracting)
        {
            currentPixelSize = 1;
            needsRedraw = true;
        }
        else if(changesLightColor)
        {
            currentPixelSize = 4;
            needsRedraw = true;
        }

        lastPos[0] = position[0]; lastPos[1] = position[1]; lastPos[2] = position[2];
        lastRot[0] = rotations[0]; lastRot[1] = rotations[1]; lastRot[2] = rotations[2];
        lastScale = scale;
        lastParamA = paramA;
        lastParamB = paramB;
        lastParamC = paramC;
        lastM = shininessM;


        wasInteracting = isInteracting || changesLightColor;
        changesLightColor = false;


        if (needsRedraw)
        {
            Mat4 S_inverse = Mat4::scale_inverse(Vect3(scale, scale, scale));
            Mat4 R_inverse = Mat4::rotateX_inverse(rotations[0]) * Mat4::rotateY_inverse(rotations[1]) * Mat4::rotateZ_inverse(rotations[2]);
            Mat4 T_inverse = Mat4::translate_inverse(Vect3(position[0], position[1], position[2]));
            Mat4 invM = S_inverse * R_inverse * T_inverse;


            Mat4 D(0.0f);
            D.table[0] = paramA;
            D.table[5] = paramB;
            D.table[10] = paramC;
            D.table[15] = -1.0f;

            Mat4 D_M_prime = invM.transpose() * D * invM;

            auto startRender = std::chrono::high_resolution_clock::now();

            renderCPU(frameBuffer, winWidth, winHeight, currentPixelSize, objectColor, lightColor, shininessM, D_M_prime);

            auto endRender = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = endRender - startRender;
            lastFrameTime = elapsed.count();

            glBindTexture(GL_TEXTURE_2D, screenTexture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());

            needsRedraw = false;
        }


        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);


        changesLightColor = false;
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &screenTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}