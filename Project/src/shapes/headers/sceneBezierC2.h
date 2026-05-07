#pragma once
#include "sceneBezier.h"
#include "scenePoint.h"
#include "VirtualPoints.h"

class SceneBezierC2 : public SceneBezier, public VirtualPoints {
public:
    BezierBasisMode currentBasis = BezierBasisMode::B_SPLINE;

    SceneBezierC2(std::string n, Transformations spawnTransform);

    void DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode) override;
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx) override;
    void Draw(Shader& shader) override;
    void Draw(Shader& shader, Mat4 parentMatrix) override;

    void markAffectedDeBoorPoints();

private:
    void UpdateVirtualPointsIfNeeded(const PreviewContext& ctx) override;
    std::vector<Vect3> calculateBernsteinPointsFrom(const std::vector<Vect3>& deBoor);

    void rebuildAllVirtualPoints(const std::vector<Vect3>& liveD);
    void addVirtualPoints(int oldNumD, int newNumD, const std::vector<Vect3>& liveD);
    void updateAffectedVirtualPoints(const std::vector<int>& dirtyDeBoorIndices, const std::vector<Vect3>& liveD);


    BezierBasisMode lastBasis = BezierBasisMode::B_SPLINE;
};