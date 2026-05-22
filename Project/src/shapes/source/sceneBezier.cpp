#include "sceneBezier.h"
#include "screenInteractions.h"
#include <algorithm>

SceneBezier::SceneBezier(std::string n, Transformations spawnTransform, ObjectType type)
        : SceneObject(std::move(n), spawnTransform, type) {}

void SceneBezier::Init()
{
    glGenVertexArrays(1, &VAO_bezier);
    glGenBuffers(1, &VBO_bezier);

    glBindVertexArray(VAO_bezier);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_bezier);
    glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &VAO_poly);
    glGenBuffers(1, &VBO_poly);
}

SceneBezier::~SceneBezier()
{
    if (VAO_bezier) glDeleteVertexArrays(1, &VAO_bezier);
    if (VAO_poly) glDeleteVertexArrays(1, &VAO_poly);
    if (VBO_poly) glDeleteBuffers(1, &VBO_poly);
}

void SceneBezier::cleanExpiredPoints()
{
    points.erase(std::remove_if(points.begin(), points.end(),
                                [](const std::weak_ptr<ScenePoint>& wp) { return wp.expired(); }), points.end());
}


void SceneBezier::RenderGeometryMode(const std::vector<Vect3>& flatPoints, Shader& shader, Mat4 VP, int winWidth, int winHeight, BezierBasisMode basis)
{
    bool isBSpline = (basis == BezierBasisMode::B_SPLINE);

    if (flatPoints.size() < (isBSpline ? 4 : 2)) return;


    glBindVertexArray(VAO_bezier);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDraw());

    int step = isBSpline ? 1 : 3;


    int limit = isBSpline ? (int)flatPoints.size() - 3 : (int)flatPoints.size() - 1;

    for (int i = 0; i < limit; i += step)
    {
        int degree = 3;
        if (!isBSpline)
        {
            int remaining = (int)flatPoints.size() - i;
            if (remaining == 3)
                degree = 2;
            if (remaining == 2)
                degree = 1;
        }

        std::vector<Vect3> segPts;
        for (int j = 0; j <= degree; ++j)
        {
            segPts.push_back(flatPoints[i + j]);
        }

        // Padowanie dla GL_LINES_ADJACENCY (musi być 4)
        while(segPts.size() < 4)
            segPts.push_back(segPts.back());

        int segments = calculateAdaptiveSegments(degree, segPts, VP, winWidth, winHeight, 199, 199);

        float data[12] = {
                segPts[0].x, segPts[0].y, segPts[0].z,
                segPts[1].x, segPts[1].y, segPts[1].z,
                segPts[2].x, segPts[2].y, segPts[2].z,
                segPts[3].x, segPts[3].y, segPts[3].z
        };
        glBindBuffer(GL_ARRAY_BUFFER, VBO_bezier);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data), data);

        shader.setInt("degree", degree);
        shader.setInt("segmentCount", segments);
        glDrawArrays(GL_LINES_ADJACENCY, 0, 4);
    }
}

void SceneBezier::RenderLineStripMode(const std::vector<Vect3>& flatPoints, Shader& shader, Mat4 VP, int winWidth, int winHeight, BezierBasisMode basis)
{
    bool isBSpline = (basis == BezierBasisMode::B_SPLINE);

    if (flatPoints.size() < (isBSpline ? 4 : 2)) return;


    glBindVertexArray(VAO_bezier);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDraw());

    int step = isBSpline ? 1 : 3;
    int limit = isBSpline ? (int)flatPoints.size() - 3 : (int)flatPoints.size() - 1;

    for (int i = 0; i < limit; i += step)
    {
        int degree = 3;
        if (!isBSpline)
        {
            int remaining = (int)flatPoints.size() - i;
            if (remaining == 3)
                degree = 2;
            if (remaining == 2)
                degree = 1;
        }

        std::vector<Vect3> segPts;
        for (int j = 0; j <= degree; ++j)
        {
            segPts.push_back(flatPoints[i + j]);
        }

        int segments = calculateAdaptiveSegments(degree, segPts, VP, winWidth, winHeight, 1024, 128);


        for (int j = 0; j < segPts.size(); ++j)
        {
            std::string loc = "p[" + std::to_string(j) + "]";
            shader.setVec3(loc, segPts[j].x, segPts[j].y, segPts[j].z);
        }

        shader.setInt("degree", degree);
        shader.setInt("segmentCount", segments);

        glDrawArrays(GL_LINE_STRIP, 0, segments + 1);
    }
}



void SceneBezier::RenderPolygon(const std::vector<Vect3>& polyPoints, Shader& lineShader) const
{
    if (!showPolygon || polyPoints.size() < 2)
        return;

    std::vector<float> data;
    data.reserve(polyPoints.size() * 3); // Drobna optymalizacja pamięci

    for(auto& pos : polyPoints)
    {
        data.push_back(pos.x);
        data.push_back(pos.y);
        data.push_back(pos.z);
    }

    glBindVertexArray(VAO_poly);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_poly);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float gray[3] = {0.4f, 0.4f, 0.4f};
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, gray);
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "model"), 1, GL_FALSE, id.table);

    glDrawArrays(GL_LINE_STRIP, 0, polyPoints.size());
}

int SceneBezier::calculateAdaptiveSegments(int degree, const std::vector<Vect3>& segPts, Mat4 VP, int winWidth, int winHeight, int maxSegments, int fallbackBehindCamera)
{
    if (degree <= 1) return 1;

    float lenPixels = 0.0f;
    std::vector<std::pair<float, float>> screenPts;
    bool isBehindCamera = false;

    for (auto& p : segPts)
    {
        float screenX, screenY;
        if (projectWorldToScreen(p, VP, winWidth, winHeight, screenX, screenY))
            screenPts.push_back({screenX, screenY});
        else
            isBehindCamera = true;
    }

    for (size_t j = 1; j < screenPts.size(); ++j)
    {
        float dx = screenPts[j].first - screenPts[j - 1].first;
        float dy = screenPts[j].second - screenPts[j - 1].second;
        lenPixels += std::sqrt(dx * dx + dy * dy);
    }

    int rawSegments = (int)(lenPixels / 4.0f);

    if (isBehindCamera)
        rawSegments = std::max(rawSegments, fallbackBehindCamera);

    return std::clamp(rawSegments, 4, maxSegments);
}