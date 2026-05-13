#pragma once
#include "sceneObject.h"
#include "scenePoint.h"
#include "previewContext.h"
#include <vector>
#include <memory>

class SceneSurface : public SceneObject {
public:
    int sizeU = 0, sizeV = 0;
    int samplesU = 4, samplesV = 4;
    bool isCylinder = false;
    bool showPolygon = true;

    std::vector<std::weak_ptr<ScenePoint>> points;

    SceneSurface(std::string n, Transformations spawnTransform, ObjectType type);
    virtual ~SceneSurface();

    void Init() override;

    // Wirtualna metoda - dzieci zadecydują, jakiego shadera użyć
    virtual void DrawSurface(Shader& shader, const PreviewContext& ctx) = 0;

    // Metoda do rysowania pomocniczych linii wieloboku
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx);

    void Draw(Shader& shader) override {}
    void Draw(Shader& shader, Mat4 parentMatrix) override {}

protected:
    unsigned int VAO_surface = 0, VBO = 0, EBO_surface = 0;
    unsigned int VAO_poly = 0, EBO_poly = 0;

    std::vector<unsigned int> patchIndices; // Dla teselacji
    std::vector<unsigned int> polyIndices;  // Dla siatki kontrolnej

    void RenderSurfaceInternal(Shader& shader, const PreviewContext& ctx);
};

// --- KLASY POCHODNE ---

class SceneSurfaceC0 : public SceneSurface {
public:
    SceneSurfaceC0(std::string n, Transformations spawnTransform)
            : SceneSurface(std::move(n), spawnTransform, ObjectType::BezierSurfaceC0) {}

    void DrawSurface(Shader& shader, const PreviewContext& ctx) override {
        RenderSurfaceInternal(shader, ctx);
    }
};

class SceneSurfaceC2 : public SceneSurface {
public:
    SceneSurfaceC2(std::string n, Transformations spawnTransform)
            : SceneSurface(std::move(n), spawnTransform, ObjectType::BezierSurfaceC2) {}

    void DrawSurface(Shader& shader, const PreviewContext& ctx) override {
        RenderSurfaceInternal(shader, ctx);
    }
};