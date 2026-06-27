#include "axis.h"

Axis::Axis() {}

Axis::~Axis()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
}

void Axis::Init()
{
    float axisBidirectionalVertices[] = {
            -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, // X
            0.0f,-1.0f, 0.0f,  0.0f, 1.0f, 0.0f, // Y
            0.0f, 0.0f,-1.0f,  0.0f, 0.0f, 1.0f  // Z
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisBidirectionalVertices), axisBidirectionalVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0); 
}

void Axis::Draw(Shader& shader, const Vect3& position, const Vect3& rotations, float length)
{
    Mat4 T  = Mat4::translate(position);
    Mat4 Rz = Mat4::rotateZ(rotations.z);
    Mat4 Ry = Mat4::rotateY(rotations.y);
    Mat4 S  = Mat4::scale(Vect3(length, length, length));

    glBindVertexArray(VAO);

    Mat4 modelZ = T * S;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, modelZ.table);
    float blue[3] = {0.1f, 0.3f, 0.8f};
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, blue);
    glDrawArrays(GL_LINES, 4, 2);

    T *= Rz;
    Mat4 modelY = T * S;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, modelY.table);
    float green[3] = {0.0f, 0.5f, 0.0f};
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, green);
    glDrawArrays(GL_LINES, 2, 2);

    T *= Ry;
    Mat4 modelX = T * S;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, modelX.table);
    float red[3] = {0.5f, 0.0f, 0.0f};
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, red);
    glDrawArrays(GL_LINES, 0, 2);

    glBindVertexArray(0);
}