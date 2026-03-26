#pragma once
#include "sceneObject.h"
#include "torusGrid.h"
#include "matrixesModelViewProjection.h"
#include "MG1Math/Mat4.h"
#include <vector>
#include <memory>


class Cursor {
public:
    Transformations transform;
    float screenX = 0.0f, screenY = 0.0f;
    unsigned int VAO=0, VBO=0;


    ~Cursor()
    {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
    }

    void Init()
    {
        float lines[] =
                {
                -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, // X
                0.0f,-1.0f, 0.0f,  0.0f, 1.0f, 0.0f, // Y
                0.0f, 0.0f,-1.0f,  0.0f, 0.0f, 1.0f  // Z
        };
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void Draw(Shader& shader)
    {
        //Mat4 modelMat = createModelMatrix(transform.getPosition(), transform.getRotation(), transform.scale);
        Mat4 modelMat = createModelMatrix(transform.getPosition(), transform.rotation, transform.scale);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, modelMat.table);
        float color[3] = { 1.0f, 0.0f, 1.0f }; // Różowy kursor dla odróżnienia [cite: 7]
        glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, color);
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, 6);
    }
};

// Zmienna globalna lub wewnątrz int main()
Cursor cursor;
std::vector<std::shared_ptr<SceneObject>> sceneObjects;