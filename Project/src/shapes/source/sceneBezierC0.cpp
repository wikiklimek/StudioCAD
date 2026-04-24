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


    std::vector<Vect3> flatPoints;
    for (auto& wp : points)
    {
        flatPoints.push_back(getPreviewPosition(wp.lock(), ctx));
    }


    if (mode == GEOMETRY)
        RenderGeometryMode(flatPoints, shader, VP, winWidth, winHeight, BezierBasisMode::BERNSTEIN);
    else
        RenderLineStripMode(flatPoints, shader, VP, winWidth, winHeight, BezierBasisMode::BERNSTEIN);
}

void SceneBezierC0::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    if (points.empty())
    {
        return;
    }


    std::vector<Vect3> flatPoints;
    for (auto& wp : points)
    {
        flatPoints.push_back(getPreviewPosition(wp.lock(), ctx));
    }

    RenderPolygon(flatPoints, lineShader);
}