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


// WSPÓLNE FUNKCJE POMOCNICZE BAZY do metody init

void SceneSurface::InitBuffers()
{
    if (VAO_surface == 0) glGenVertexArrays(1, &VAO_surface);
    if (VAO_poly == 0) glGenVertexArrays(1, &VAO_poly);
    if (VBO == 0) glGenBuffers(1, &VBO);
    if (EBO_surface == 0) glGenBuffers(1, &EBO_surface);
    if (EBO_poly == 0) glGenBuffers(1, &EBO_poly);

    patchIndices.clear();
}

void SceneSurface::InitPolygonAndUpload()
{
    polyIndices.clear();

    // Linie poziome
    for (int v = 0; v < sizeV; ++v)
    {
        for (int u = 0; u < sizeU - 1; ++u)
        {
            polyIndices.push_back(v * sizeU + u);
            polyIndices.push_back(v * sizeU + u + 1);
        }
    }
    // Linie pionowe
    for (int u = 0; u < sizeU; ++u)
    {
        for (int v = 0; v < sizeV - 1; ++v)
        {
            polyIndices.push_back(v * sizeU + u);
            polyIndices.push_back((v + 1) * sizeU + u);
        }
    }

    // Ładowanie VBO
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


void SceneSurfaceC0::Init()
{
    InitBuffers();

    int patchesU = (sizeU - 1) / 3;
    int patchesV = (sizeV - 1) / 3;

    for (int pv = 0; pv < patchesV; ++pv)
    {
        for (int pu = 0; pu < patchesU; ++pu)
        {
            int startU = pu * 3;
            int startV = pv * 3;

            for (int j = 0; j < 4; ++j)
            {
                for (int i = 0; i < 4; ++i)
                {
                    patchIndices.push_back((startV + j) * sizeU + (startU + i));
                }
            }
        }
    }

    InitPolygonAndUpload();
}


void SceneSurfaceC2::Init()
{
    InitBuffers();

    int patchesV = sizeV - 3;
    int patchesU = isCylinder ? (sizeU - 1) : (sizeU - 3);

    for (int pv = 0; pv < patchesV; ++pv)
    {
        for (int pu = 0; pu < patchesU; ++pu)
        {
            int startU = pu;
            int startV = pv;

            for (int j = 0; j < 4; ++j)
            {
                for (int i = 0; i < 4; ++i)
                {
                    int u_idx = startU + i;

                    // C2 CYLINDER
                    if (isCylinder)
                    {
                        if (u_idx >= sizeU)
                        {
                            u_idx = u_idx - sizeU + 1;
                        }
                    }

                    patchIndices.push_back((startV + j) * sizeU + u_idx);
                }
            }
        }
    }

    InitPolygonAndUpload();
}

void SceneSurface::RenderSurfaceInternal(Shader& shader, const PreviewContext& ctx)
{
    if (points.empty() || patchIndices.empty()) return;

    std::vector<Vect3> currentPositions;
    currentPositions.reserve(points.size());
    for (auto& wp : points)
    {
        if (auto p = wp.lock())
            currentPositions.push_back(getPreviewPosition(p, ctx));
        else
            currentPositions.push_back(Vect3(0.0f));
    }


    bool needsUpload = false;
    if (currentPositions.size() != prevPositions.size())
    {
        needsUpload = true;
    }
    else
    {
        for (size_t i = 0; i < currentPositions.size(); ++i)
        {
            if (currentPositions[i].x != prevPositions[i].x ||
                currentPositions[i].y != prevPositions[i].y ||
                currentPositions[i].z != prevPositions[i].z)
            {
                needsUpload = true;
                break; // Znaleźliśmy zmianę, uciekamy
            }
        }
    }


    if (needsUpload)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, currentPositions.size() * sizeof(Vect3), currentPositions.data());

        prevPositions = currentPositions;
    }

    shader.use();
    glUniform1i(glGetUniformLocation(shader.ID, "u_tessLevelU"), samplesU);
    glUniform1i(glGetUniformLocation(shader.ID, "u_tessLevelV"), samplesV);

    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, id.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, color);

    glBindVertexArray(VAO_surface);
    glPatchParameteri(GL_PATCH_VERTICES, 16);

    // stałe v, rysowanie krzywych poziomych
    glUniform1i(glGetUniformLocation(shader.ID, "swapUV"), 0);
    glDrawElements(GL_PATCHES, patchIndices.size(), GL_UNSIGNED_INT, 0);

    // stałe u, rysiwania krzywych pionowych
    glUniform1i(glGetUniformLocation(shader.ID, "swapUV"), 1);
    glDrawElements(GL_PATCHES, patchIndices.size(), GL_UNSIGNED_INT, 0);
}


void SceneSurface::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    if (!showPolygon || points.empty() || polyIndices.empty()) return;

    // Pozycje są już w VBO zaktualizowane przez RenderSurfaceInterna bo było wczesniej wywołane

    lineShader.use();
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "model"), 1, GL_FALSE, id.table);

    float gray[3] = {0.4f, 0.4f, 0.4f};
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, gray);

    glBindVertexArray(VAO_poly);
    glDrawElements(GL_LINES, polyIndices.size(), GL_UNSIGNED_INT, 0);
}