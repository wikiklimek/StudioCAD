#pragma once
#include "sceneTorus.h"

#include <utility>

SceneTorus::SceneTorus(std::string n, Transformations spawnTransform)
        : SceneObject(std::move(n), spawnTransform, ObjectType::Torus) {}

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

void SceneTorus::Draw(Shader& shader)  {
    Mat4 localMat = createModelMatrix(transformations.getPosition(), transformations.rotation, transformations.scale);
    Mat4 finalMat = localMat;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, finalMat.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDraw());

    // trimming
    glUniform1f(glGetUniformLocation(shader.ID, "torusR"), R); // Promień Torusa niezbędny do u/v
    glUniform1i(glGetUniformLocation(shader.ID, "useTrim"), useTrim ? 1 : 0);
    glUniform1i(glGetUniformLocation(shader.ID, "trimFlip"), trimFlip ? 1 : 0);
    if (useTrim) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, trimTexture);
        glUniform1i(glGetUniformLocation(shader.ID, "trimMap"), 1);
        glActiveTexture(GL_TEXTURE0);
    }


    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
}

void SceneTorus::Draw(Shader& shader, Mat4 parentMatrix)  {
    Mat4 localMat = createModelMatrix(transformations.getPosition(), transformations.rotation, transformations.scale);
    Mat4 finalMat = parentMatrix * localMat;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, finalMat.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDraw());

    // trymowanie
    glUniform1f(glGetUniformLocation(shader.ID, "torusR"), R); // Promień Torusa niezbędny do u/v
    glUniform1i(glGetUniformLocation(shader.ID, "useTrim"), useTrim ? 1 : 0);
    glUniform1i(glGetUniformLocation(shader.ID, "trimFlip"), trimFlip ? 1 : 0);
    if (useTrim) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, trimTexture);
        glUniform1i(glGetUniformLocation(shader.ID, "trimMap"), 1);
        glActiveTexture(GL_TEXTURE0);
    }


    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
}



Vect3 SceneTorus::EvaluatePos(float u, float v) const
{
    //  u, v z [0, 1] na pełne [0, 2PI]
    float u_rad = u * 2.0f * (float)M_PI;
    float v_rad = v * 2.0f * (float)M_PI;

    float x = (R + r * std::cos(v_rad)) * std::cos(u_rad);
    float y = (R + r * std::cos(v_rad)) * std::sin(u_rad);
    float z = r * std::sin(v_rad);

    Mat4 modelMat = createModelMatrix(transformations.getPosition(), transformations.rotation, transformations.scale);
    Vect4 localPos(x, y, z, 1.0f);

    return (modelMat * localPos).toVect3();
}

Vect3 SceneTorus::EvaluateDu(float u, float v) const
{
    float u_rad = u * 2.0f * (float)M_PI;
    float v_rad = v * 2.0f * (float)M_PI;

    float dx = -(R + r * std::cos(v_rad)) * std::sin(u_rad);
    float dy =  (R + r * std::cos(v_rad)) * std::cos(u_rad);
    float dz = 0.0f;

    Mat4 dirMat = createModelMatrix(Vect3(0.0f, 0.0f, 0.0f), transformations.rotation, transformations.scale);
    Vect4 localDir(dx, dy, dz, 0.0f);

    return (dirMat * localDir).toVect3() * (2.0f * (float)M_PI);
}

Vect3 SceneTorus::EvaluateDv(float u, float v) const
{
    float u_rad = u * 2.0f * (float)M_PI;
    float v_rad = v * 2.0f * (float)M_PI;

    float dx = -r * std::sin(v_rad) * std::cos(u_rad);
    float dy = -r * std::sin(v_rad) * std::sin(u_rad);
    float dz =  r * std::cos(v_rad);

    Mat4 dirMat = createModelMatrix(Vect3(0.0f, 0.0f, 0.0f), transformations.rotation, transformations.scale);
    Vect4 localDir(dx, dy, dz, 0.0f);

    return (dirMat * localDir).toVect3() * (2.0f * (float)M_PI);
}