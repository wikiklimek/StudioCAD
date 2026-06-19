#pragma once
#include "scenePoint.h"
#include "matrixesModelViewProjection.h"

#include <utility>



ScenePoint::ScenePoint(std::string n, Transformations spawnTransform)
        : SceneObject(std::move(n), spawnTransform, ObjectType::Point), size(4.0f) {}

ScenePoint::ScenePoint(std::string n, float s, Transformations spawnTransform)
        : SceneObject(std::move(n), spawnTransform, ObjectType::Point), size(s) {}

ScenePoint::~ScenePoint() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
}

bool ScenePoint::isAnyWaySelected()
{
    return isSelected || (selectedCurvesCount > 0) || (selectedSurfacesCount > 0) || isSelectedAsDeBoore;
}

void ScenePoint::Init()  {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    float origin[] = {0.0f, 0.0f, 0.0f};

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(origin), origin, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void ScenePoint::Draw(Shader& shader) {
    Mat4 localMat = createModelMatrix(transformations.getPosition(), transformations.rotation, transformations.scale);
    Mat4 finalMat = localMat;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, finalMat.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDraw());
    glBindVertexArray(VAO);
    glPointSize(size);
    glDrawArrays(GL_POINTS, 0, 1);
}

void ScenePoint::DrawAtPosition(Shader& shader, Vect3 Position) {
    Mat4 localMat = createModelMatrix(Position, Quaternion(), 1.0);
    Mat4 finalMat = localMat;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, finalMat.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDraw());
    glBindVertexArray(VAO);
    glPointSize(size);
    glDrawArrays(GL_POINTS, 0, 1);
}

void ScenePoint::Draw(Shader& shader, Mat4 parentMatrix) {
    Mat4 localMat = createModelMatrix(transformations.getPosition(), transformations.rotation, transformations.scale);
    Mat4 finalMat = parentMatrix * localMat;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, finalMat.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDraw());
    glBindVertexArray(VAO);
    glPointSize(size);
    glDrawArrays(GL_POINTS, 0, 1);
}
