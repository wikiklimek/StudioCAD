#pragma once
#include "scenePoint.h"
#include "previewContext.h"
#include <vector>
#include <memory>

class SceneBezierC0 : public SceneObject {
public:
    std::vector<std::weak_ptr<ScenePoint>> points;
    bool showPolygon = false;

    unsigned int VAO_bezier = 0;
    unsigned int VAO_poly = 0;
    unsigned int VBO_poly = 0;

    SceneBezierC0(std::string n, Transformations spawnTransform);
    ~SceneBezierC0() override;

    void Init() override;
    void Draw(Shader& shader) override {}
    void Draw(Shader& shader, Mat4 parentMatrix) override {}

    // Zamiast tony argumentów, przyjmują PreviewContext
    void DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx);
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx);
};