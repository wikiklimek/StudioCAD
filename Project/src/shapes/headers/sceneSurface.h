#pragma once
#include "sceneObject.h"
#include "scenePoint.h"
#include "previewContext.h"
#include "scenePolygon.h"
#include <vector>
#include <memory>

class SceneSurface : public SceneObject, public ScenePolygon {
public:
    int sizeU = 0, sizeV = 0;
    int samplesU = 4, samplesV = 4;

    bool isCylinder = false;
    bool showPolygon = true;

    std::vector<std::weak_ptr<ScenePoint>> points;
    std::vector<Vect3> prevPositions;

    SceneSurface(std::string n, Transformations spawnTransform, ObjectType type);
    virtual ~SceneSurface();

    void Init() override = 0;

    virtual void DrawSurface(Shader& shader, const PreviewContext& ctx) = 0;
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx) override;

    void Draw(Shader& shader) override {}
    void Draw(Shader& shader, Mat4 parentMatrix) override {}

protected:
    unsigned int VAO_surface = 0, VBO = 0, EBO_surface = 0;
    unsigned int VAO_poly = 0, EBO_poly = 0;

    std::vector<unsigned int> patchIndices;
    std::vector<unsigned int> polyIndices;

    void RenderSurfaceInternal(Shader& shader, const PreviewContext& ctx);

    void InitBuffers();
    void InitPolygonAndUpload();
};


class SceneSurfaceC0 : public SceneSurface {
public:
    SceneSurfaceC0(std::string n, Transformations spawnTransform)
            : SceneSurface(std::move(n), spawnTransform, ObjectType::BezierSurfaceC0) {}

    void Init() override;
    void DrawSurface(Shader& shader, const PreviewContext& ctx) override {
        RenderSurfaceInternal(shader, ctx);
    }
};

class SceneSurfaceC2 : public SceneSurface {
public:
    SceneSurfaceC2(std::string n, Transformations spawnTransform)
            : SceneSurface(std::move(n), spawnTransform, ObjectType::BezierSurfaceC2) {}

    void Init() override;
    void DrawSurface(Shader& shader, const PreviewContext& ctx) override {
        RenderSurfaceInternal(shader, ctx);
    }
};