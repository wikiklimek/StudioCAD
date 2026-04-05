#pragma once
#include "sceneObject.h"
#include "scenePoint.h"
#include <vector>
#include <memory>

class SceneBezierC0 : public SceneObject {
public:
    std::vector<std::weak_ptr<ScenePoint>> points;
    bool showPolygon = false;

    // Rozdzielone uchwyty na pamięć GPU
    unsigned int VAO_bezier = 0;
    unsigned int VAO_poly = 0;
    unsigned int VBO_poly = 0;

    SceneBezierC0(std::string n, Transformations spawnTransform);
    ~SceneBezierC0() override;

    void Init() override;
    void Draw(Shader& shader) override {}
    void Draw(Shader& shader, Mat4 parentMatrix) override {}

    // Specjalistyczne funkcje rysujące
    void DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight,
                    bool isTransforming, bool isLocal, Vect3 localDelta, Mat4 groupMat, bool transformAll);

    void DrawPolygon(Shader& lineShader,
                     bool isTransforming, bool isLocal, Vect3 localDelta, Mat4 groupMat, bool transformAll);
};