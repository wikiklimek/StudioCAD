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

    // Główna funkcja orkiestrująca
    std::vector<Vect3> getLiveBernsteinPoints(const PreviewContext& ctx);

    // Konwerter z De Boora na Bernsteina
    std::vector<Vect3> calculateBernsteinPointsFrom(const std::vector<Vect3>& deBoor);

    std::vector<std::shared_ptr<ScenePoint>> virtualPoints;


    // --- CACHE DO OPTYMALIZACJI I ZAMRAŻANIA RUCHU ---
    std::vector<Vect3> lastDeBoorPositions;
    std::vector<Vect3> dragStartDeBoor; // ZAMROŻONA BAZA STARTOWA
    bool wasTransforming = false;
    BezierBasisMode lastBasis = BezierBasisMode::B_SPLINE;
};