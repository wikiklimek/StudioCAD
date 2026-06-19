#include "sceneIntersectionCurve.h"
#include <glad/glad.h>

SceneIntersectionCurve::SceneIntersectionCurve(std::string n, const std::vector<IntersectionPoint>& pts)
        : SceneObject(std::move(n), Transformations(), ObjectType::IntersectionCurve), // <--- Używa nowego Enuma!
          intersectionPoints(pts)
{
    color[0] = 0.0f; color[1] = 0.0f; color[2] = 0.0f; // Domyślnie czarna linia przecięcia
}

SceneIntersectionCurve::~SceneIntersectionCurve() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
}

void SceneIntersectionCurve::Init() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    std::vector<Vect3> flatPoints;
    for (const auto& p : intersectionPoints) {
        flatPoints.push_back(p.worldPos);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, flatPoints.size() * sizeof(Vect3), flatPoints.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);
}

void SceneIntersectionCurve::Draw(Shader& shader) {
    shader.use();
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, id.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDrawBasedOn(isSelected));

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINE_STRIP, 0, intersectionPoints.size());
}

void SceneIntersectionCurve::Draw(Shader& shader, Mat4 parentMatrix) {
    Draw(shader); // Nasza krzywa i tak jest już w World Space
}

// Konwersja na Interpolacyjną Krzywą Sklejaną (Wymóg zadania 10)
std::shared_ptr<SceneSplineInterpolating> SceneIntersectionCurve::convertToSpline(std::vector<std::shared_ptr<SceneObject>>& sceneObjects, int pointStep)
{
    auto spline = std::make_shared<SceneSplineInterpolating>(name + " (Spline)", Transformations());
    spline->Init();

    for (size_t i = 0; i < intersectionPoints.size(); i += pointStep) {
        auto pt = std::make_shared<ScenePoint>("P_int_" + std::to_string(i), intersectionPoints[i].worldPos);
        pt->Init();
        sceneObjects.push_back(pt);

        spline->points.push_back(pt);
        pt->globalCurvesCount++;
    }

    if ((intersectionPoints.size() - 1) % pointStep != 0) {
        auto pt = std::make_shared<ScenePoint>("P_int_end", intersectionPoints.back().worldPos);
        pt->Init();
        sceneObjects.push_back(pt);
        spline->points.push_back(pt);
        pt->globalCurvesCount++;
    }

    return spline;
}