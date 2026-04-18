#include "sceneBezierC0.h"
#include "previewFunctions.h"

SceneBezierC0::SceneBezierC0(std::string n, Transformations spawnTransform)
        : SceneBezier(std::move(n), spawnTransform, ObjectType::BezierCurveC0) {}

void SceneBezierC0::DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode)
{
    cleanExpiredPoints();

    if (points.empty())
    {
        pendingDelete = true;
        return;
    }

    // Wyciągamy fizyczne pozycje punktów
    std::vector<Vect3> flatPoints;
    for (auto& wp : points)
    {
        flatPoints.push_back(getPreviewPosition(wp.lock(), ctx));
    }

    // Delegujemy rysowanie do klasy bazowej!
    if (mode == GEOMETRY)
        RenderGeometryMode(flatPoints, shader, VP, winWidth, winHeight);
    else
        RenderLineStripMode(flatPoints, shader, VP, winWidth, winHeight);
}

void SceneBezierC0::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    cleanExpiredPoints();

    if (points.empty())
    {
        pendingDelete = true;
        return;
    }

    // Wyciągamy fizyczne pozycje punktów
    std::vector<Vect3> flatPoints;
    for (auto& wp : points)
    {
        flatPoints.push_back(getPreviewPosition(wp.lock(), ctx));
    }

    // Odpalamy wspólną logikę!
    RenderPolygon(flatPoints, lineShader);
}