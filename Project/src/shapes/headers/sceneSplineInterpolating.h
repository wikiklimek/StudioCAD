#pragma once
#include "sceneBezier.h"

class SceneSplineInterpolating : public SceneBezier {
public:
    std::vector<std::shared_ptr<ScenePoint>> virtualPoints;
    InterpolationBasisMode currentBasis = InterpolationBasisMode::ALGEBRAIC;

    struct Coeffs {
        Vect3 a, b, c, d_coeff;
        float d_len;
    };
    std::vector<Coeffs> segmentCoeffs;

    SceneSplineInterpolating(std::string n, Transformations spawnTransform);

    void UpdateStructuresIfNeeded(const PreviewContext& ctx);
    void DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode) override;
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx) override;
    void Draw(Shader& shader) override;
    void Draw(Shader& shader, Mat4 parentMatrix) override;
};