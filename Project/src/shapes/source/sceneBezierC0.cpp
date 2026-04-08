#pragma once
#include "sceneBezierC0.h"
#include "previewFunctions.h"
#include "screenInteractions.h"
#include <algorithm>

SceneBezierC0::SceneBezierC0(std::string n, Transformations spawnTransform)
        : SceneObject(std::move(n), spawnTransform, ObjectType::BezierCurveC0) {}

void SceneBezierC0::Init()
{
    glGenVertexArrays(1, &VAO_bezier);

    // 2. Osobne VAO i VBO dla łamanej, żeby nie niszczyły pamięci krzywej
    glGenVertexArrays(1, &VAO_poly);
    glGenBuffers(1, &VBO_poly);
}

SceneBezierC0::~SceneBezierC0() {
    if (VAO_bezier) glDeleteVertexArrays(1, &VAO_bezier);
    if (VAO_poly) glDeleteVertexArrays(1, &VAO_poly);
    if (VBO_poly) glDeleteBuffers(1, &VBO_poly);
}

void SceneBezierC0::DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx)
{
    points.erase(std::remove_if(points.begin(), points.end(),
                                [](const std::weak_ptr<ScenePoint>& wp) { return wp.expired(); }), points.end());

    if (points.empty())
    {
        pendingDelete = true;
        return;
    }


    glBindVertexArray(VAO_bezier);
    //shader.use();
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, color);

    for (size_t i = 0; i < points.size() - 1; i += 3)
    {
        size_t remaining = points.size() - i;
        int degree = 1;
        if (remaining >= 4)
            degree = 3;
        else if (remaining == 3)
            degree = 2;

        std::vector<Vect3> segPts;
        for (int j = 0; j <= degree; ++j)
        {
            segPts.push_back(getPreviewPosition(points[i + j].lock(), ctx));
        }

        int segments = 1;

        if (degree > 1)
        {
            float lenPixels = 0.0f;
            std::vector<std::pair<float, float>> screenPts;
            bool isBehindCamera = false;

            // 1. Rzutujemy punkty i sprawdzamy, czy któryś uciekł
            for (auto& p : segPts)
            {
                float screenX, screenY;
                if (projectWorldToScreen(p, VP, winWidth, winHeight, screenX, screenY))
                {
                    screenPts.push_back({screenX, screenY});
                }
                else
                {
                    isBehindCamera = true;
                }
            }

            // 2. ZAWSZE liczymy długość łamanej, ale tylko dla punktów, które ocalały!
            // Jeśli zostały np. 3 z 4 punktów, zsumujemy odległość między nimi na ekranie.
            for (size_t j = 1; j < screenPts.size(); ++j)
            {
                float dx = screenPts[j].first - screenPts[j-1].first;
                float dy = screenPts[j].second - screenPts[j-1].second;
                lenPixels += std::sqrt(dx*dx + dy*dy);
            }

            // 3. Wstępne wyliczenie (adaptacyjne)
            int rawSegments = (int)(lenPixels / 4.0f);

            // 4. NASZ NOWY DOPALACZ: Jeśli krzywa przecina kamerę, podnosimy stawkę.
            if (isBehindCamera)
            {
                // Bierzemy to, co WIEKSZE: wyliczone segmenty z pikseli ALBO bezpieczne 100 dla bliskich obiektów
                rawSegments = std::max(rawSegments, 256);
            }

            // 5. Ostateczny, żelazny limit (Clamp), żeby karta nie spłonęła (od 20 do 200)
            segments = std::clamp(rawSegments, 16, 4096);
        }

        for (int j = 0; j <= degree; ++j)
        {
            std::string loc = "p[" + std::to_string(j) + "]";
            shader.setVec3(loc, segPts[j].x, segPts[j].y, segPts[j].z);
        }
        shader.setInt("degree", degree);
        shader.setInt("segmentCount", segments);

        glDrawArrays(GL_LINE_STRIP, 0, segments + 1);
    }
}

void SceneBezierC0::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    if (points.empty()) {
        pendingDelete = true;
        return;
    }

    //nie rysuje dla 2 punktow bo by sie linie nakladaly
    if (!showPolygon || points.size() < 3) return;


    std::vector<float> data;
    for(auto& wp : points)
    {
        Vect3 pos = getPreviewPosition(wp.lock(), ctx);
        data.push_back(pos.x); data.push_back(pos.y); data.push_back(pos.z);
    }

    glBindVertexArray(VAO_poly);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_poly);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //lineShader.use();
    float gray[3] = {0.4f, 0.4f, 0.4f};
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, gray);
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "model"), 1, GL_FALSE, id.table);

    glDrawArrays(GL_LINE_STRIP, 0, points.size());
}