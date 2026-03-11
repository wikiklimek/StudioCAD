#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>

// --- TWOJE BIBLIOTEKI MATEMATYCZNE ---
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
// 2. SILNIK RENDERUJĄCY NA PROCESORZE (CPU)
// ==============================================================================
// ZOPTYMALIZOWANY: Przyjmuje wyliczoną raz macierz D_M_prime
void renderCPU(std::vector<unsigned char>& buffer, int width, int height, int pixelSize,
               Vect3 objectColor, Vect3 lightColor, float m, const Mat4& D_M_prime)
{
    // Symetryczna macierz potrzebna do obliczania wektora normalnego (obliczana raz na klatkę!)
    Mat4 D_M_prime_sym = D_M_prime + D_M_prime.transpose();

    Vect3 bgColor(0.15f, 0.15f, 0.15f);

    // Wyłuskanie wartości do zmiennych lokalnych
    float p00 = D_M_prime.table[0]; float p10 = D_M_prime.table[1]; float p20 = D_M_prime.table[2]; float p30 = D_M_prime.table[3];
    float p01 = D_M_prime.table[4]; float p11 = D_M_prime.table[5]; float p21 = D_M_prime.table[6]; float p31 = D_M_prime.table[7];
    float p02 = D_M_prime.table[8]; float p12 = D_M_prime.table[9]; float p22 = D_M_prime.table[10];float p32 = D_M_prime.table[11];
    float p03 = D_M_prime.table[12];float p13 = D_M_prime.table[13];float p23 = D_M_prime.table[14];float p33 = D_M_prime.table[15];

    float viewScale = 5.0f;
    float aspectRatio = (float)width / (float)height;

    for (int y = 0; y < height; y += pixelSize) {
        for (int x = 0; x < width; x += pixelSize) {
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

                // Obliczanie normalnej
                Vect4 normal4 = D_M_prime_sym * hitPointWorld4;
                Vect3 worldNormal = Vect3(normal4.x, normal4.y, normal4.z).normalize();

                // Oświetlenie Phonga (tylko specular, obserwator w +Z patrzący na -Z)
                Vect3 viewDir(0.0f, 0.0f, 1.0f);
                float w_dot_n = std::max(Vect3::dot(viewDir, worldNormal), 0.0f);
                float specularTerm = std::pow(w_dot_n, m);

                finalColor = (lightColor * specularTerm) * objectColor;
            }

            unsigned char cr = (unsigned char)(std::min(finalColor.x, 1.0f) * 255.0f);
            unsigned char cg = (unsigned char)(std::min(finalColor.y, 1.0f) * 255.0f);
            unsigned char cb = (unsigned char)(std::min(finalColor.z, 1.0f) * 255.0f);

            // Zapisz do bloku pikseli (uwzględniając pixelSize)
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

// --- Enumy do sterowania ---
enum DragMode { NONE, TRANSLATE, ROTATE_X, ROTATE_Y, ROTATE_Z, SCALE, ROTATE_FREE };

int main() {
    // ==========================================
    // INICJALIZACJA OPENGL, GLFW I IMGUI
    // ==========================================
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1024, 768, "CAD Ellipsoid Raytracer", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    Shader shader("src/shaders/raycasting_cpu.vs", "src/shaders/raycasting_cpu.fs");

    // Pełnoekranowy quad
    float vertices[] = { 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

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

    // ==========================================
    // ZMIENNE APLIKACJI I STANU
    // ==========================================
    float objectColor[3] = { 1.0f, 0.5f, 0.2f };
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    float paramA = 1.0f, paramB = 1.0f, paramC = 1.0f;
    float shininessM = 10.0f;

    float position[3] = {0.0f, 0.0f, 0.0f};
    float scale[3]    = {1.0f, 1.0f, 1.0f};
    Mat4 objectRotation(1.0f); // Pamięta historię wszystkich obrotów

    // NOWE ZMIENNE DO ADAPTACYJNEGO RENDEROWANIA:
    int currentPixelSize = 1;
    double lastFrameTime = 0.0; // czas rysowania poprzedniej klatki w sekundach
    double targetFrameTime = 1.0 / 30.0; // celujemy w 30 FPS (0.033s)

    bool isDragging = false;
    DragMode currentMode = NONE;
    double lastMouseX = 0, lastMouseY = 0;

    // ==========================================
    // GŁÓWNA PĘTLA RENDERUJĄCA
    // ==========================================
    while (!glfwWindowShouldClose(window))
    {
        // Aktualizacja rozmiaru okna
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

        // --- 1. OBLICZANIE MACIERZY (POCZĄTEK KLATKI) ---
        Mat4 S_inverse = Mat4::scale_inverse(Vect3(scale[0], scale[1], scale[2]));
        Mat4 R_inverse = objectRotation.transpose();
        Mat4 T_inverse = Mat4::translate_inverse(Vect3(position[0], position[1], position[2]));
        Mat4 invM = S_inverse * R_inverse * T_inverse;

        Mat4 D(0.0f);
        D.table[0] = paramA; D.table[5] = paramB; D.table[10] = paramC; D.table[15] = -1.0f;
        Mat4 D_M_prime = invM.transpose() * D * invM;

        // --- 2. OBSŁUGA MYSZY I INTERAKCJI ---
        ImGuiIO& io = ImGui::GetIO();
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        bool isClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        // Sprawdzanie kliknięcia tylko wtedy, gdy mysz nie jest nad oknem ImGui

            if (isClick) //&& !isDragging)
            {
                // Czy kliknęliśmy precyzyjnie w wyrenderowaną elipsoidę?
                // if (checkHit((float)mouseX, (float)mouseY, winWidth, winHeight, D_M_prime))
                {
                    isDragging = true;
                    // Rozpoznawanie wciśniętego klawisza modyfikującego
                    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
                        currentMode = TRANSLATE;
                    else if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) currentMode = ROTATE_FREE;
                    else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) currentMode = ROTATE_X;
                    else if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) currentMode = ROTATE_Y;
                    else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) currentMode = ROTATE_Z;
                    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) currentMode = SCALE;
                    else currentMode = NONE;
                }
            }


        // Zwolnienie przycisku myszy kończy interakcję
        if (!isClick)
        {
            isDragging = false;
            currentMode = NONE;
        }

        // LOGIKA TRANSFORAMCJI PODCZAS RUCHU MYSZKĄ
        if (isDragging && currentMode != NONE)
        {
            float dx_screen = (float)(mouseX - lastMouseX);
            float dy_screen = (float)(mouseY - lastMouseY);

            float viewScale = 5.0f;
            float aspectRatio = (float)winWidth / (float)winHeight;

            // Konwersja wektora ruchu myszy z pikseli na wirtualne jednostki świata
            float dx_world = (dx_screen / (float)winWidth) * 2.0f * aspectRatio * viewScale;
            float dy_world = (dy_screen / (float)winHeight) * 2.0f * viewScale;

            if (currentMode == TRANSLATE)
            {
                // Translacja w przestrzeni globalnej (podąża za ekranem)
                position[0] += dx_world;
                position[1] += dy_world;
            }
            else if (currentMode == ROTATE_X )
            {
                float angle = -dy_world;
                Mat4 deltaRot = Mat4::rotateX(angle);

                // LOKALNE
                //objectRotation = objectRotation * deltaRot;

                // GLOBALNE
                objectRotation = deltaRot * objectRotation;
            }
            else if ( currentMode == ROTATE_Y )
            {
                float angle = dx_world;
                Mat4 deltaRot = Mat4::rotateY(angle);

                // LOKALNE
                //objectRotation = objectRotation * deltaRot;

                // GLOBALNE
                objectRotation = deltaRot * objectRotation;
            }
            else if (currentMode == ROTATE_Z)
            {
                if(mouseX != lastMouseX || mouseY != lastMouseY)
                {
                    float x_world = (mouseX / (float) winWidth) * 2.0f * aspectRatio * viewScale;
                    float y_world = (mouseY / (float) winHeight) * 2.0f * viewScale;

                    float x_world_last = (lastMouseX / (float) winWidth) * 2.0f * aspectRatio * viewScale;
                    float y_world_last = (lastMouseY / (float) winHeight) * 2.0f * viewScale;

                    float length_old = sqrt(x_world_last * x_world_last + y_world_last * y_world_last);
                    float length_new = sqrt(x_world * x_world + y_world * y_world);
                    float scalar = abs(x_world_last * x_world + y_world_last * y_world);

                    float angle = 0;
                    if (length_old != 0 && length_new != 0)
                    {
                        angle = acos(scalar / (length_old * length_new)) / 3.14;

                        //float angle = -sqrt(dx_world * dx_world + dy_world * dy_world);
                        Mat4 deltaRot = Mat4::rotateZ(angle);

                        // LOKALNE
                        //objectRotation = objectRotation * deltaRot;

                        // GLOBALNE
                        objectRotation = deltaRot * objectRotation;
                    }
                }
            }
            else if (currentMode == SCALE)
            {
                // Rzutowanie ruchu myszy na wyświetlane osie lokalne elipsoidy.
                // Wyłuskujemy kierunki osi z macierzy rotacji.
                float localXx = objectRotation.table[0]; float localXy = objectRotation.table[1];
                float localYx = objectRotation.table[4]; float localYy = objectRotation.table[5];
                float localZx = objectRotation.table[8]; float localZy = objectRotation.table[9];

                // Iloczyn skalarny: Skala powiększy się tylko, gdy mysz poruszy się wzdłuż danej osi
                scale[0] += dx_world * localXx + dy_world * localXy;
                scale[1] += dx_world * localYx + dy_world * localYy;
                scale[2] += dx_world * localZx + dy_world * localZy;

                // Ograniczenie, aby nie przeskalować obiektu na minus
                scale[0] = std::max(0.01f, scale[0]);
                scale[1] = std::max(0.01f, scale[1]);
                scale[2] = std::max(0.01f, scale[2]);



            }
            else if (currentMode == ROTATE_FREE)
            {
                // Mysz góra/dół obraca wokół osi X
                float angleX = -dy_world;
                Mat4 deltaRotX = Mat4::rotateX(angleX);

                // Mysz lewo/prawo obraca wokół osi Y
                float angleY = dx_world;
                Mat4 deltaRotY = Mat4::rotateY(angleY);

                // Składamy oba obroty w jedną macierz
                Mat4 deltaRot = deltaRotY * deltaRotX;

                // GLOBALNE - aplikujemy do obiektu
                objectRotation = deltaRot * objectRotation;
            }

            // UWAGA: Myszka zmieniła wartości obiektu! Musimy natychmiast przeliczyć
            // macierz D_M_prime, żeby do silnika CPU trafiły zaktualizowane dane.
            S_inverse = Mat4::scale_inverse(Vect3(scale[0], scale[1], scale[2]));
            R_inverse = objectRotation.transpose();
            T_inverse = Mat4::translate_inverse(Vect3(position[0], position[1], position[2]));
            invM = S_inverse * R_inverse * T_inverse;
            D_M_prime = invM.transpose() * D * invM;
        }

        lastMouseX = mouseX;
        lastMouseY = mouseY;

        // --- 3. IMGUI INTERFEJS ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool imguiChanged = false; // Flaga sprawdzająca ruch suwakami

        ImGui::Begin("Sterowanie Elipsoida (CAD Mode)");
        ImGui::Text("Kliknij W MODEL + przeciagnij:");
        ImGui::BulletText("[SHIFT] - Przesuniecie");
        ImGui::BulletText("[X], [Y], [Z] - Obrot lokalny");
        ImGui::BulletText("[S] - Skalowanie");
        ImGui::Separator();

        if (ImGui::DragFloat3("Pozycja (XYZ)", position, 0.05f)) imguiChanged = true;
        if (ImGui::DragFloat3("Skala (XYZ)", scale, 0.05f, 0.1f, 10.0f)) imguiChanged = true;
        ImGui::Separator();

        if (ImGui::DragFloat("Parametr a", &paramA, 0.01f, 0.1f, 10.0f)) imguiChanged = true;
        if (ImGui::DragFloat("Parametr b", &paramB, 0.01f, 0.1f, 10.0f)) imguiChanged = true;
        if (ImGui::DragFloat("Parametr c", &paramC, 0.01f, 0.1f, 10.0f)) imguiChanged = true;
        ImGui::Separator();

        if (ImGui::ColorEdit3("Kolor obiektu", objectColor)) imguiChanged = true;
        if (ImGui::ColorEdit3("Kolor swiatla", lightColor)) imguiChanged = true;
        if (ImGui::SliderFloat("Potega (m)", &shininessM, 1.0f, 128.0f)) imguiChanged = true;

        // Możesz wyświetlić aktualny rozmiar piksela tylko do podglądu (jako tekst, a nie suwak)
        ImGui::Text("Rozmiar piksela: %d", currentPixelSize);
        ImGui::Text("Czas renderu CPU: %.3f ms", lastFrameTime * 1000.0);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();

        // --- 4. LOGIKA ADAPTACYJNA I RENDEROWANIE ---

        // Sprawdzamy, czy w tej klatce wystąpiła jakakolwiek interakcja (ruch myszą po scenie LUB suwaki)
        bool isInteracting = (isDragging && currentMode != NONE) || imguiChanged;

        if (isInteracting) {
            // Jeśli interakcja trwa, próbujemy utrzymać targetFrameTime (np. 30 FPS)
            double margin = 0;//0.005; // 5ms marginesu błędu

            if (lastFrameTime > targetFrameTime + margin) {
                currentPixelSize++; // Komputer nie wyrabia - zwiększamy pikselozę
                if (currentPixelSize > 16) currentPixelSize = 16; // limit maksymalny
            }
            else if (lastFrameTime < targetFrameTime - margin) {
                currentPixelSize--; // Mamy zapas mocy - zmniejszamy pikselozę
                if (currentPixelSize < 2) currentPixelSize = 2; // nie schodzimy poniżej 2 w trakcie ruchu
            }
        } else {
            // Brak interakcji -> pełna precyzja okienka
            currentPixelSize = 1;
        }

        // Mierzymy czas samego renderowania na CPU
        auto startRender = std::chrono::high_resolution_clock::now();

        // Wywołujemy Twój renderer przekazując wyliczony currentPixelSize
        renderCPU(frameBuffer, winWidth, winHeight, currentPixelSize, objectColor, lightColor, shininessM, D_M_prime);

        auto endRender = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endRender - startRender;
        lastFrameTime = elapsed.count(); // Zapisujemy na potrzeby następnej klatki

        // --- AKTUALIZACJA TEKSTURY I OPENGL (Bez zmian) ---
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, winWidth, winHeight, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer.data());

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        shader.use();

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // --- SPRZĄTANIE ---
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