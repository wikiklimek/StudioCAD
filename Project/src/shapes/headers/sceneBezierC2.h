#pragma once
#include "sceneBezier.h"
#include "scenePoint.h"

class SceneBezierC2 : public SceneBezier {
public:
    SceneBezierC2(std::string n, Transformations spawnTransform);

    void DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode) override;
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx) override;

    void Draw(Shader& shader) override;
    void Draw(Shader& shader, Mat4 parentMatrix) override;

    BezierBasisMode currentBasis = BezierBasisMode::B_SPLINE;

    void UpdateVirtualPointsIfNeeded(const PreviewContext& ctx);
    std::vector<Vect3> calculateBernsteinPointsFrom(const std::vector<Vect3>& deBoor);

    // TWOJA FUNKCJA DO OZNACZANIA DE BOORÓW!
    void markAffectedDeBoorPoints();

    std::vector<std::shared_ptr<ScenePoint>> virtualPoints;
    BezierBasisMode lastBasis = BezierBasisMode::B_SPLINE;
};