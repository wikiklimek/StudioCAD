#pragma once
#include "sceneTorus.h"

#include <utility>

SceneTorus::SceneTorus(std::string n, Transformations spawnTransform) : SceneObject(std::move(n), spawnTransform) {}

SceneTorus::~SceneTorus() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void SceneTorus::Init() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    UpdateBuffers();
}

void SceneTorus::UpdateBuffers() {
    generateTorus(R, r, density_R, density_r, vertices, indices);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void SceneTorus::Draw(Shader& shader, Mat4 parentMatrix)  {
    Mat4 localMat = createModelMatrix(transformations.getPosition(), transformations.rotation, transformations.scale);
    Mat4 finalMat = parentMatrix * localMat;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, finalMat.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, color);
    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
}