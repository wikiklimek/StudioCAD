#pragma once
#include "cursor.h"

Cursor::~Cursor()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
}

void Cursor::Init()
{
    float lines[] =
            {
                    0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, // X
                    0.0f,0.0f, 0.0f,  0.0f, 1.0f, 0.0f, // Y
                    0.0f, 0.0f,0.0f,  0.0f, 0.0f, 1.0f  // Z
            };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}


void Cursor::Draw(Shader& shader)
{
    Mat4 modelMat = createModelMatrix(transform.getPosition(), transform.rotation, transform.scale);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, modelMat.table);
    glBindVertexArray(VAO);

    // Oś X
    float colorX[3] = { 1.0f, 0.0f, 0.0f };
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, colorX);
    glDrawArrays(GL_LINES, 0, 2);

    // Oś Y
    float colorY[3] = { 0.0f, 1.0f, 0.0f };
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, colorY);
    glDrawArrays(GL_LINES, 2, 2);

    // Oś Z
    float colorZ[3] = { 0.1f, 0.5f, 1.0f };
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, colorZ);
    glDrawArrays(GL_LINES, 4, 2);
}