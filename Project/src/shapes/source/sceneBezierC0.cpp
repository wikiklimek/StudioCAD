#pragma once
#include "sceneBezierC0.h"
#include <algorithm>

SceneBezierC0::SceneBezierC0(std::string n, Transformations spawnTransform)
        : SceneObject(std::move(n), spawnTransform, ObjectType::BezierCurveC0) {} // <-- Przekazujemy typ!

void SceneBezierC0::Init() {
    // 1. Czyste VAO dla krzywej.
    // Nowoczesny OpenGL nie potrzebuje VBO, jeśli shader czyta tylko gl_VertexID!
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

    // --- TWOJA MAGIA: LAMBDA WYLICZAJĄCA ZMANIPULOWANĄ POZYCJĘ ---
    auto getPreviewPos = [&](std::shared_ptr<ScenePoint> p) -> Vect3 {
        Vect3 pos = p->transformations.getPosition();

        if (ctx.isTransforming && (ctx.isEntireScene || p->isSelected || p->selectedCurvesCount > 0)) {
            if (ctx.isLocal) {
                pos.x += ctx.localDeltaPos.x;
                pos.y += ctx.localDeltaPos.y;
                pos.z += ctx.localDeltaPos.z;
            } else {
                Vect4 p4(pos.x, pos.y, pos.z, 1.0f);
                Vect4 newP = ctx.groupMat * p4;
                pos = Vect3(newP.x, newP.y, newP.z);
            }
        }
        return pos;
    };

    glBindVertexArray(VAO_bezier);
    shader.use();
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, color);

    for (size_t i = 0; i < points.size() - 1; i += 3) {
        size_t remaining = points.size() - i;
        int degree = 1;
        if (remaining >= 4) degree = 3;
        else if (remaining == 3) degree = 2;

        std::vector<Vect3> segPts;
        for (int j = 0; j <= degree; ++j) {
            // TUTAJ UŻYWAMY LAMBDY ZAMIAST CZYSTEJ POZYCJI!
            segPts.push_back(getPreviewPos(points[i + j].lock()));
        }

        float lenPixels = 0.0f;
        std::vector<std::pair<float, float>> screenPts;
        for (auto& p : segPts) {
            Vect4 p4(p.x, p.y, p.z, 1.0f);
            Vect4 clip = VP * p4;
            if (clip.w > 0.0001f) {
                float nx = clip.x / clip.w;
                float ny = clip.y / clip.w;
                screenPts.push_back({(nx + 1.0f) / 2.0f * winWidth, (1.0f - ny) / 2.0f * winHeight});
            }
        }

        for (size_t j = 1; j < screenPts.size(); ++j) {
            float dx = screenPts[j].first - screenPts[j-1].first;
            float dy = screenPts[j].second - screenPts[j-1].second;
            lenPixels += std::sqrt(dx*dx + dy*dy);
        }

        int segments = std::max(1, (int)(lenPixels / 10.0f));

        for (int j = 0; j <= degree; ++j) {
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
    if (!showPolygon || points.size() < 2) return;

    auto getPreviewPos = [&](std::shared_ptr<ScenePoint> p) -> Vect3 {
        Vect3 pos = p->transformations.getPosition();

        if (ctx.isTransforming && (ctx.isEntireScene || p->isSelected || p->selectedCurvesCount > 0)) {
            if (ctx.isLocal) {
                pos.x += ctx.localDeltaPos.x;
                pos.y += ctx.localDeltaPos.y;
                pos.z += ctx.localDeltaPos.z;
            } else {
                Vect4 p4(pos.x, pos.y, pos.z, 1.0f);
                Vect4 newP = ctx.groupMat * p4;
                pos = Vect3(newP.x, newP.y, newP.z);
            }
        }
        return pos;
    };

    std::vector<float> data;
    for(auto& wp : points) {
        // TUTAJ TEŻ UŻYWAMY LAMBDY!
        Vect3 pos = getPreviewPos(wp.lock());
        data.push_back(pos.x); data.push_back(pos.y); data.push_back(pos.z);
    }

    glBindVertexArray(VAO_poly);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_poly);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    lineShader.use();
    float gray[3] = {0.5f, 0.5f, 0.5f};
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, gray);
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "model"), 1, GL_FALSE, id.table);

    glDrawArrays(GL_LINE_STRIP, 0, points.size());
}