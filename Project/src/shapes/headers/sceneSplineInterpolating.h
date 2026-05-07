#pragma once
#include "sceneBezier.h"
#include "VirtualPoints.h"

class SceneSplineInterpolating : public SceneBezier, public VirtualPoints {
public:
    InterpolationBasisMode currentBasis = InterpolationBasisMode::ALGEBRAIC;

    SceneSplineInterpolating(std::string n, Transformations spawnTransform);

    void DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode) override;
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx) override;
    void Draw(Shader& shader) override;
    void Draw(Shader& shader, Mat4 parentMatrix) override;

private:
    void UpdateVirtualPointsIfNeeded(const PreviewContext& ctx) override;

    struct Coeffs {
        Vect3 a, b, c, d_coeff;
        float d_len;
    };
    std::vector<Coeffs> segmentCoeffs;
};

