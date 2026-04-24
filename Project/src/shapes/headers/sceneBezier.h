#pragma once
#include "sceneObject.h"
#include "scenePoint.h"
#include "shader.h"
#include "previewContext.h"
#include <vector>
#include <memory>
#include <string>


class SceneBezier : public SceneObject {
public:
    SceneBezier(std::string n, Transformations spawnTransform, ObjectType type);
    virtual ~SceneBezier();

    virtual void Init() override;

    //moze w  przyszłosci jakos to sprytnie opprawimy
    void Draw(Shader& shader) override {}
    void Draw(Shader& shader, Mat4 parentMatrix) override {}

    std::vector<std::weak_ptr<ScenePoint>> points;
    bool showPolygon = true;

    // Funkcje rysujące. Słowo 'virtual' oznacza, że dzieci (C0 i C2) muszą je napisać po swojemu.
    // 'override' połączymy w klasach pochodnych.
    virtual void DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode) = 0;
    virtual void DrawPolygon(Shader& lineShader, const PreviewContext& ctx) = 0;

protected:

    unsigned int VAO_bezier = 0, VBO_bezier = 0;
    unsigned int VAO_poly = 0, VBO_poly = 0;


    void cleanExpiredPoints();


    void RenderGeometryMode(const std::vector<Vect3>& flatPoints, Shader& shader, Mat4 VP, int winWidth, int winHeight, BezierBasisMode basis);
    void RenderLineStripMode(const std::vector<Vect3>& flatPoints, Shader& shader, Mat4 VP, int winWidth, int winHeight, BezierBasisMode basis);

    void RenderPolygon(const std::vector<Vect3>& polyPoints, Shader& lineShader) const;
    static int calculateAdaptiveSegments(int degree, const std::vector<Vect3>& segPts, Mat4 VP, int winWidth, int winHeight, int maxSegments, int fallbackBehindCamera);
};