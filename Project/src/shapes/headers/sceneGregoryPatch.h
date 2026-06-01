#pragma once
#include "sceneObject.h"
#include "scenePoint.h"
#include "previewContext.h"
#include <vector>
#include <memory>
#include <array>

class SceneGregoryPatch : public SceneObject {
public:
    // Przechowujemy dokładnie 60 punktów (3 sub-płaty po 20 punktów)
    std::vector<std::weak_ptr<ScenePoint>> points;

    // Zmienne do kontrolowania gęstości siatki NIEZALEŻNIE dla każdego z 3 sub-płatów
    std::array<int, 3> samplesU = {4, 4, 4};
    std::array<int, 3> samplesV = {4, 4, 4};

    bool showPolygon = false; // Czy rysować szkielet (wektory styczne)
    bool showVectors = false; // Nowa flaga do GUI

    // Bufory
    unsigned int VAO_surface = 0, VBO = 0;
    GLuint VAO_vectors = 0;
    unsigned int VAO_poly = 0, EBO_poly = 0;
    GLuint VBO_vectors = 0;

    std::vector<unsigned int> polyIndices;
    std::vector<Vect3> prevPositions;

    SceneGregoryPatch(std::string n, Transformations spawnTransform);
    ~SceneGregoryPatch() override;

    void Init() override;

    // Dedykowane metody rysujące (wywoływane w main.cpp tak samo jak w SceneSurface)
    void DrawSurface(Shader& shader, const PreviewContext& ctx);
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx);
    void DrawVectors(Shader& lineShader, const PreviewContext& ctx);

    // Zaślepki dla metod bazowych SceneObject
    void Draw(Shader& shader) override {}
    void Draw(Shader& shader, Mat4 parentMatrix) override {}

private:
    void InitBuffers();
    void InitPolygonIndices();
};