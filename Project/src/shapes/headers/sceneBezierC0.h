#pragma once
#include "sceneBezier.h"

class SceneBezierC0 : public SceneBezier {
public:
    SceneBezierC0(std::string n, Transformations spawnTransform);

    // Musimy dostarczyć implementacje funkcji rysujących, więc dodajemy 'override'
    void DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode) override;
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx) override;
};