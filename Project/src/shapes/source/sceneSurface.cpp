#include "sceneSurface.h"
#include "previewFunctions.h"
#include <glad/glad.h>

SceneSurface::SceneSurface(std::string n, Transformations spawnTransform, ObjectType type)
        : SceneObject(std::move(n), spawnTransform, type) {}

SceneSurface::~SceneSurface()
{
    if (VAO_surface) glDeleteVertexArrays(1, &VAO_surface);
    if (VAO_poly) glDeleteVertexArrays(1, &VAO_poly);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO_surface) glDeleteBuffers(1, &EBO_surface);
    if (EBO_poly) glDeleteBuffers(1, &EBO_poly);
}

void SceneSurface::Init()
{
    if (VAO_surface == 0) glGenVertexArrays(1, &VAO_surface);
    if (VAO_poly == 0) glGenVertexArrays(1, &VAO_poly);
    if (VBO == 0) glGenBuffers(1, &VBO);
    if (EBO_surface == 0) glGenBuffers(1, &EBO_surface);
    if (EBO_poly == 0) glGenBuffers(1, &EBO_poly);

    // 1. INDEKSY DLA TESELACJI (PATCHES)
    patchIndices.clear();
    int patchesU = (objectType == ObjectType::BezierSurfaceC0) ? (sizeU - 1) / 3 : sizeU - 3;
    int patchesV = (objectType == ObjectType::BezierSurfaceC0) ? (sizeV - 1) / 3 : sizeV - 3;

    for (int pv = 0; pv < patchesV; ++pv) {
        for (int pu = 0; pu < patchesU; ++pu) {
            int startU = (objectType == ObjectType::BezierSurfaceC0) ? pu * 3 : pu;
            int startV = (objectType == ObjectType::BezierSurfaceC0) ? pv * 3 : pv;
            for (int j = 0; j < 4; ++j) {
                for (int i = 0; i < 4; ++i) {
                    patchIndices.push_back((startV + j) * sizeU + (startU + i));
                }
            }
        }
    }

    // 2. INDEKSY DLA WIELOBOKU KONTROLNEGO (Siatka poziomo-pionowa)
    polyIndices.clear();
    // Linie poziome
    for (int v = 0; v < sizeV; ++v) {
        for (int u = 0; u < sizeU - 1; ++u) {
            polyIndices.push_back(v * sizeU + u);
            polyIndices.push_back(v * sizeU + u + 1);
        }
        // Domknięcie walca dla siatki
        if (isCylinder) {
            polyIndices.push_back(v * sizeU + (sizeU - 1));
            polyIndices.push_back(v * sizeU + 0);
        }
    }
    // Linie pionowe
    for (int u = 0; u < sizeU; ++u) {
        for (int v = 0; v < sizeV - 1; ++v) {
            polyIndices.push_back(v * sizeU + u);
            polyIndices.push_back((v + 1) * sizeU + u);
        }
    }

    // 3. REZERWACJA PAMIĘCI VBO (Używamy jednego VBO dla obu VAO)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Vect3), nullptr, GL_DYNAMIC_DRAW);

    // Konfiguracja VAO_surface (Teselacja)
    glBindVertexArray(VAO_surface);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_surface);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, patchIndices.size() * sizeof(unsigned int), patchIndices.data(), GL_STATIC_DRAW);

    // Konfiguracja VAO_poly (Wielobok)
    glBindVertexArray(VAO_poly);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_poly);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyIndices.size() * sizeof(unsigned int), polyIndices.data(), GL_STATIC_DRAW);
}

// BAZOWA METODA RYSOWANIA POWIERZCHNI
void SceneSurface::RenderSurfaceInternal(Shader& shader, const PreviewContext& ctx)
{
    if (points.empty() || patchIndices.empty()) return;

    // Przeliczenie pozycji (Myszka / Transformacje)
    std::vector<Vect3> currentPositions;
    currentPositions.reserve(points.size());
    for (auto& wp : points) {
        if (auto p = wp.lock()) currentPositions.push_back(getPreviewPosition(p, ctx));
        else currentPositions.push_back(Vect3(0.0f));
    }

    // Wrzutka do VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, currentPositions.size() * sizeof(Vect3), currentPositions.data());

    shader.use();
    glUniform1i(glGetUniformLocation(shader.ID, "u_tessLevelU"), samplesU);
    glUniform1i(glGetUniformLocation(shader.ID, "u_tessLevelV"), samplesV);

    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, id.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, color);

    glBindVertexArray(VAO_surface);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPatchParameteri(GL_PATCH_VERTICES, 16);
    glDrawElements(GL_PATCHES, patchIndices.size(), GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// RYSOWANIE SIATKI (WIELOBOKU) ZWYKŁYMI LINIAMI
void SceneSurface::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    if (!showPolygon || points.empty() || polyIndices.empty()) return;

    // Pozycje są już w VBO zaktualizowane przez RenderSurfaceInternal
    // więc nie musimy ich liczyć ponownie, wystarczy narysować!

    lineShader.use();
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "model"), 1, GL_FALSE, id.table);

    float gray[3] = {0.4f, 0.4f, 0.4f};
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, gray);

    glBindVertexArray(VAO_poly);
    glDrawElements(GL_LINES, polyIndices.size(), GL_UNSIGNED_INT, 0);
}