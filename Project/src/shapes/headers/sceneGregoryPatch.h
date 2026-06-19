#pragma once
#include "sceneObject.h"
#include "scenePoint.h"
#include "previewContext.h"
#include <vector>
#include <memory>
#include <array>

//przygotowane typowo pod 3 þłaty - łatwo zmienic
class SceneGregoryPatch : public SceneObject, public std::enable_shared_from_this<SceneGregoryPatch> {
public:

    std::vector<std::shared_ptr<ScenePoint>> points;
    std::vector<std::weak_ptr<ScenePoint>> bezierPatchPoints;
    std::vector<std::weak_ptr<ScenePoint>> bezierPatchPointsInner;
    size_t id_gregory = 0;


    std::array<int, 3> samplesU = {4, 4, 4};
    std::array<int, 3> samplesV = {4, 4, 4};

    bool showPolygon = false;
    bool showVectors = false;


    unsigned int VAO_surface = 0, VBO = 0;
    GLuint VAO_vectors = 0;
    unsigned int VAO_poly = 0, EBO_poly = 0;
    GLuint VBO_vectors = 0;

    std::vector<unsigned int> polyIndices;
    std::vector<Vect3> prevPositions;

    SceneGregoryPatch(std::string n, Transformations spawnTransform);
    ~SceneGregoryPatch() override;

    void Init() override;


    void DrawSurface(Shader& shader, const PreviewContext& ctx);
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx);
    void DrawVectors(Shader& lineShader, const PreviewContext& ctx);


    void Draw(Shader& shader) override {}
    void Draw(Shader& shader, Mat4 parentMatrix) override {}

private:
    void InitBuffers();
    void InitPolygonIndices();
    void UpdateIfNeeded(const PreviewContext &ctx);
};