#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// TWOJE BIBLIOTEKI MATEMATYCZNE
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

// ==============================================================================
// SILNIK RENDERUJĄCY NA PROCESORZE (CPU)
// ==============================================================================
void renderCPU(std::vector<unsigned char>& buffer, int width, int height, int pixelSize,
               float a, float b, float c,
               Vect3 objectColor, Vect3 lightColor, float m, Mat4 inverseTransformMatrix
               )
{
    // 1. Odwracamy transformacje (Podejście B)
    Mat4 invTransform = inverseTransformMatrix;//transformMatrix.inverse();
    Mat4 normalMatrix = invTransform.transpose();

    Vect3 bgColor(0.15f, 0.15f, 0.15f); // Kolor tła

    // Pętla po ekranie z przeskokiem o "rozmiar piksela" (Optymalizacja!)
    for (int y = 0; y < height; y += pixelSize)
    {
        for (int x = 0; x < width; x += pixelSize)
        {

            float px = x + pixelSize * 0.5f;
            float py = height - 1 - (y + pixelSize * 0.5f);

            // 1. Liczymy proporcje ekranu (np. 1024 / 768 = ok. 1.33)
            float aspectRatio = (float)width / (float)height;

// 2. Mnożymy u przez proporcje, żeby rozszerzyć wirtualny ekran w poziomie!
            float u = ((px / (float)width) * 2.0f - 1.0f) * aspectRatio;
            float v = (py / (float)height) * 2.0f - 1.0f;

// 3. Dodajemy kontrolę nad perspektywą (Ogniskowa / Zoom)
// Im wyższa wartość (np. 2.0, 3.0), tym mniejszy kąt widzenia i mniejsze wygięcie przestrzeni
            float focalLength = 3.5f;

// Kamera i promień (Teraz z poprawną optyką!)
            Vect3 rayOriginWorld(0.0f, 0.0f, 3.0f);
            Vect3 rayDirWorld = Vect3(u, v, -focalLength).normalize();

            // =========================================================
            // TRANSFORMACJA PROMIENIA WSTECZ (w=1 dla punktów, w=0 dla kierunków)
            // =========================================================
            Vect4 ro4 = invTransform * Vect4(rayOriginWorld.x, rayOriginWorld.y, rayOriginWorld.z, 1.0f);
            Vect4 rd4 = invTransform * Vect4(rayDirWorld.x, rayDirWorld.y, rayDirWorld.z, 0.0f);

            Vect3 rayOrigin = ro4.toVect3();
            Vect3 rayDir = rd4.toVect3().normalize();

            // =========================================================
            // MATEMATYKA ZDERZENIA Z KWADRYKĄ
            // =========================================================
            float Aq = a * rayDir.x * rayDir.x + b * rayDir.y * rayDir.y + c * rayDir.z * rayDir.z;
            float Bq = 2.0f * (a * rayOrigin.x * rayDir.x + b * rayOrigin.y * rayDir.y + c * rayOrigin.z * rayDir.z);
            float Cq = a * rayOrigin.x * rayOrigin.x + b * rayOrigin.y * rayOrigin.y + c * rayOrigin.z * rayOrigin.z - 1.0f;

            float delta = Bq * Bq - 4.0f * Aq * Cq;
            Vect3 finalColor = bgColor;

            if (delta >= 0.0f)
            {
                float t = (-Bq - std::sqrt(delta)) / (2.0f * Aq);
                if (t >= 0.0f)
                {
                    // Punkt uderzenia lokalny
                    Vect3 hitPointLocal = rayOrigin + rayDir * t;

                    // Normalna lokalna (z gradientu)
                    Vect3 localNormal = Vect3(2.0f * a * hitPointLocal.x, 2.0f * b * hitPointLocal.y, 2.0f * c * hitPointLocal.z).normalize();

                    // Powrót do świata
                    Vect3 worldNormal = (normalMatrix * Vect4(localNormal.x, localNormal.y, localNormal.z, 0.0f)).toVect3().normalize();
                    Vect3 hitPointWorld = rayOriginWorld + rayDirWorld * t;

                    // Oświetlenie Phonga (Specular ONLY)
                    Vect3 viewLightDir = (rayOriginWorld - hitPointWorld).normalize();

                    float w_dot_n = std::max(Vect3::dot(viewLightDir, worldNormal), 0.0f);
                    float specularTerm = std::pow(w_dot_n, m);

                    // Mieszanie koloru z Phonga (wektor * wektor)
                    finalColor = (lightColor * specularTerm) * objectColor;
                }
            }

            // =========================================================
            // ZAPIS KOLORU DO TABLICY W RAM (Pixelation)
            // =========================================================
            unsigned char cr = (unsigned char)(std::min(finalColor.x, 1.0f) * 255.0f);
            unsigned char cg = (unsigned char)(std::min(finalColor.y, 1.0f) * 255.0f);
            unsigned char cb = (unsigned char)(std::min(finalColor.z, 1.0f) * 255.0f);

            for (int by = 0; by < pixelSize; ++by)
            {
                for (int bx = 0; bx < pixelSize; ++bx)
                {
                    if (x + bx < width && y + by < height)
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


int main() {
    // ==========================================
    // 1. INICJALIZACJA OKNA I GLAD
    // ==========================================
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Software Raytracer (CPU)", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // 2. shadery
    Shader shader("src/shaders/raycasting_cpu.vs", "src/shaders/raycasting_cpu.fs");

    // ==========================================
    // 3. VAO, VBO ORAZ EBO (Full-Screen Quad)
    // ==========================================
    float vertices[] = {
            1.0f,  1.0f, 0.0f,  // Indeks 0: Prawy górny
            1.0f, -1.0f, 0.0f,  // Indeks 1: Prawy dolny
            -1.0f, -1.0f, 0.0f, // Indeks 2: Lewy dolny
            -1.0f,  1.0f, 0.0f  // Indeks 3: Lewy górny
    };

    unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
    };

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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // ==========================================
    // 4. BUFOR EKRANU NA CPU I TEKSTURA
    // ==========================================
    int winWidth = 1024, winHeight = 768;
    std::vector<unsigned char> frameBuffer(winWidth * winHeight * 3, 0);

    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // Ostre krawędzie pikseli
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, winWidth, winHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    // ==========================================
    // 5. INICJALIZACJA IMGUI
    // ==========================================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    // --- ZMIENNE APLIKACJI ---
    float objectColor[3] = { 1.0f, 0.5f, 0.2f };
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    float paramA = 1.0f, paramB = 1.0f, paramC = 1.0f;
    float shininessM = 32.0f;
    int pixelSize = 8;

    // Transformacje (Wymagają Twojej klasy Mat4)
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[3] = {0.0f, 0.0f, 0.0f}; // Stopnie z ImGui
    float scale[3]    = {1.0f, 1.0f, 1.0f};

    // ==========================================
    // 6. GŁÓWNA PĘTLA
    // ==========================================
    while (!glfwWindowShouldClose(window))
    {
        // Sprawdzamy zmianę rozdzielczości okna
        int currentW, currentH;
        glfwGetFramebufferSize(window, &currentW, &currentH);
        if (currentW > 0 && currentH > 0 && (currentW != winWidth || currentH != winHeight)) {
            winWidth = currentW; winHeight = currentH;
            frameBuffer.resize(winWidth * winHeight * 3);
            glBindTexture(GL_TEXTURE_2D, screenTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, winWidth, winHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        }

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ------------------ UI ------------------
        ImGui::Begin("Sterowanie CPU Raytracerem");
        ImGui::Text("Transformacje obiektu:");
        ImGui::DragFloat3("Pozycja (XYZ)", position, 0.05f);
        ImGui::SliderFloat3("Obrot (Stopnie)", rotation, -180.0f, 180.0f);
        ImGui::DragFloat3("Skala (XYZ)", scale, 0.05f, 0.1f, 10.0f);
        ImGui::Separator();

        ImGui::Text("Geometria:");
        ImGui::DragFloat("Parametr a", &paramA, 0.01f, 0.1f, 10.0f);
        ImGui::DragFloat("Parametr b", &paramB, 0.01f, 0.1f, 10.0f);
        ImGui::DragFloat("Parametr c", &paramC, 0.01f, 0.1f, 10.0f);
        ImGui::Separator();

        ImGui::Text("Oswietlenie Phonga:");
        ImGui::ColorEdit3("Kolor obiektu", objectColor);
        ImGui::ColorEdit3("Kolor swiatla", lightColor);
        ImGui::SliderFloat("Potega (m)", &shininessM, 1.0f, 128.0f);
        ImGui::Separator();

        ImGui::Text("Optymalizacja CPU:");
        ImGui::SliderInt("Rozmiar Piksela", &pixelSize, 1, 30);
        ImGui::End();

        // ------------------ MATEMATYKA ------------------
        // Budujemy główną macierz (Skala -> Obrót -> Translacja)
        //Mat4 S = Mat4::scale(Vect3(scale[0], scale[1], scale[2]));
        Mat4 S_inverse = Mat4::scale_inverse(Vect3(scale[0], scale[1], scale[2]));

        // C++ przyjmuje radiany do funkcji trygonometrycznych!
        float rx = rotation[0] * 3.14159f / 180.0f;
        float ry = rotation[1] * 3.14159f / 180.0f;
        float rz = rotation[2] * 3.14159f / 180.0f;
        //Mat4 R = Mat4::rotateZ(rz) * Mat4::rotateY(ry) * Mat4::rotateX(rx);
        Mat4 R_inverse = Mat4::rotateX_inverse(rx) * Mat4::rotateY_inverse(ry) * Mat4::rotateZ_inverse(rz);

        //Mat4 T = Mat4::translate(Vect3(position[0], position[1], position[2]));
        Mat4 T_inverse = Mat4::translate_inverse(Vect3(position[0], position[1], position[2]));

        //Mat4 modelMatrix = T * R * S;
        Mat4 modelMatrix_inverse = S_inverse * R_inverse * T_inverse;

        // ------------------ RENDEROWANIE (CPU) ------------------
        renderCPU(frameBuffer, winWidth, winHeight, pixelSize,
                  paramA, paramB, paramC,
                  Vect3(objectColor[0], objectColor[1], objectColor[2]),
                  Vect3(lightColor[0], lightColor[1], lightColor[2]), shininessM,
                  //modelMatrix
                  modelMatrix_inverse
                  );

        // ------------------ WYSŁANIE NA EKRAN (GPU) ------------------
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        // W shaderze mamy uniform o nazwie "screenTexture" (domyślnie używa tekstury 0, co nam pasuje)

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ------------------ RYSOWANIE IMGUI ------------------
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // ==========================================
    // 7. SPRZĄTANIE
    // ==========================================
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    // Usunięcie wygenerowanej tekstury
    glDeleteTextures(1, &screenTexture);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}