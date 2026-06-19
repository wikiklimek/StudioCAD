#include "sceneIntersectionCurve.h"
#include <glad/glad.h>
#include <cmath>

SceneIntersectionCurve::SceneIntersectionCurve(std::string n, const std::vector<IntersectionPoint>& pts)
        : SceneObject(std::move(n), Transformations(), ObjectType::IntersectionCurve), // <--- Używa nowego Enuma!
          intersectionPoints(pts)
{
    color[0] = 0.0f; color[1] = 0.0f; color[2] = 0.0f; // Domyślnie czarna linia przecięcia
}

SceneIntersectionCurve::~SceneIntersectionCurve() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);

    if (textureA) glDeleteTextures(1, &textureA);
    if (textureB) glDeleteTextures(1, &textureB);
}

void SceneIntersectionCurve::Init() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    std::vector<Vect3> flatPoints;
    for (const auto& p : intersectionPoints) {
        flatPoints.push_back(p.worldPos);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, flatPoints.size() * sizeof(Vect3), flatPoints.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);

    GenerateParametricTextures();
}

void SceneIntersectionCurve::Draw(Shader& shader) {
    shader.use();
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, id.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDrawBasedOn(isSelected));

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINE_STRIP, 0, intersectionPoints.size());
}

void SceneIntersectionCurve::Draw(Shader& shader, Mat4 parentMatrix) {
    Draw(shader); // Nasza krzywa i tak jest już w World Space
}

// Konwersja na Interpolacyjną Krzywą Sklejaną (Wymóg zadania 10)
std::shared_ptr<SceneSplineInterpolating> SceneIntersectionCurve::convertToSpline(std::vector<std::shared_ptr<SceneObject>>& sceneObjects, int pointStep)
{
    auto spline = std::make_shared<SceneSplineInterpolating>(name + " (Spline)", Transformations());
    spline->Init();

    for (size_t i = 0; i < intersectionPoints.size(); i += pointStep) {
        auto pt = std::make_shared<ScenePoint>("P_int_" + std::to_string(i), intersectionPoints[i].worldPos);
        pt->Init();
        sceneObjects.push_back(pt);

        spline->points.push_back(pt);
        pt->globalCurvesCount++;
    }

    if ((intersectionPoints.size() - 1) % pointStep != 0) {
        auto pt = std::make_shared<ScenePoint>("P_int_end", intersectionPoints.back().worldPos);
        pt->Init();
        sceneObjects.push_back(pt);
        spline->points.push_back(pt);
        pt->globalCurvesCount++;
    }

    return spline;
}

// ---------------------------------------------------------
// Generowanie masek przestrzeni parametrów u/v
// ---------------------------------------------------------
void SceneIntersectionCurve::GenerateParametricTextures()
{
    const int res = 1024; // Rozdzielczość
    std::vector<unsigned char> bufferA(res * res * 4, 255);
    std::vector<unsigned char> bufferB(res * res * 4, 255);

    // Wzbogacona funkcja malująca z PŁYNNYM ZAWIJANIEM (Modulo)
    auto drawThickPixel = [&](std::vector<unsigned char>& buf, int cx, int cy) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {

                // Magia zawijania! (Używamy + res żeby uniknąć ujemnego modulo w C++)
                int x = ((cx + i) % res + res) % res;
                int y = ((cy + j) % res + res) % res;

                int idx = (y * res + x) * 4;
                buf[idx] = 0;     // R
                buf[idx+1] = 0;   // G
                buf[idx+2] = 0;   // B
                buf[idx+3] = 255; // A
            }
        }
    };

    // Klasyczny Bresenham (nic nie zmieniamy, po prostu wyślemy mu "wirtualne" punkty poza ekranem)
    auto drawLine = [&](std::vector<unsigned char>& buf, int x0, int y0, int x1, int y1) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;
        for (;;) {
            drawThickPixel(buf, x0, y0);
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    };

    if (intersectionPoints.size() > 1) {
        for (size_t i = 0; i < intersectionPoints.size() - 1; ++i) {

            // --- POWIERZCHNIA A ---
            // Skalujemy U/V [0, 1] na piksele [0, 1024]
            int xA0 = (int)(intersectionPoints[i].uA * res); xA0 = (xA0 % res + res) % res;
            int yA0 = (int)(intersectionPoints[i].vA * res); yA0 = (yA0 % res + res) % res;
            int xA1 = (int)(intersectionPoints[i+1].uA * res); xA1 = (xA1 % res + res) % res;
            int yA1 = (int)(intersectionPoints[i+1].vA * res); yA1 = (yA1 % res + res) % res;

            // SZUKANIE NAJKRÓTSZEJ DROGI WOBEC ZAWINIĘCIA
            // Jeśli odległość > połowa rozdzielczości, szybciej jest pójść "wirtualnie" w drugą stronę obrazka!
            if (xA1 - xA0 > res / 2) xA1 -= res;
            else if (xA0 - xA1 > res / 2) xA1 += res;

            if (yA1 - yA0 > res / 2) yA1 -= res;
            else if (yA0 - yA1 > res / 2) yA1 += res;

            drawLine(bufferA, xA0, yA0, xA1, yA1);

            // --- POWIERZCHNIA B ---
            int xB0 = (int)(intersectionPoints[i].uB * res); xB0 = (xB0 % res + res) % res;
            int yB0 = (int)(intersectionPoints[i].vB * res); yB0 = (yB0 % res + res) % res;
            int xB1 = (int)(intersectionPoints[i+1].uB * res); xB1 = (xB1 % res + res) % res;
            int yB1 = (int)(intersectionPoints[i+1].vB * res); yB1 = (yB1 % res + res) % res;

            if (xB1 - xB0 > res / 2) xB1 -= res;
            else if (xB0 - xB1 > res / 2) xB1 += res;

            if (yB1 - yB0 > res / 2) yB1 -= res;
            else if (yB0 - yB1 > res / 2) yB1 += res;

            drawLine(bufferB, xB0, yB0, xB1, yB1);
        }
    }

    auto uploadTexture = [](GLuint& tex, const std::vector<unsigned char>& buf, int r) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, r, r, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Zmienione na GL_REPEAT aby ewentualne filtrowanie zawijało się poprawnie!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    };

    uploadTexture(textureA, bufferA, res);
    uploadTexture(textureB, bufferB, res);
}