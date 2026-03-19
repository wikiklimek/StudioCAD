#pragma once
#include "axis.h"


void drawEulerAxes(Shader& shader, unsigned int axisVAO, const float position[3], const float rotations[3], float length)
{
    Vect3 pos(position[0], position[1], position[2]);

    Mat4 T  = Mat4::translate(pos);
    Mat4 Rz = Mat4::rotateZ(rotations[2]);
    Mat4 Ry = Mat4::rotateY(rotations[1]);
    Mat4 S  = Mat4::scale(Vect3(length, length, length));

    glBindVertexArray(axisVAO);

    // Z - B
    Mat4 modelZ = T * S;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, modelZ.table);
    float blue[3] = {0.0f, 0.0f, 1.0f};
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, blue);
    glDrawArrays(GL_LINES, 4, 2);

    // Y - G
    T *= Rz;
    Mat4 modelY = T * S;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, modelY.table);
    float green[3] = {0.0f, 1.0f, 0.0f};
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, green);
    glDrawArrays(GL_LINES, 2, 2);

    // Z - B
    T *= Ry;
    Mat4 modelX = T * S;
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, modelX.table);
    float red[3] = {1.0f, 0.0f, 0.0f};
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, red);
    glDrawArrays(GL_LINES, 0, 2);
}



void drawGlobalAxes(Shader& shader, unsigned int axisVAO, float length)
{
    Mat4 S = Mat4::scale(Vect3(length, length, length));

    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, S.table);
    glBindVertexArray(axisVAO);

    // X - R
    float red[3] = {0.3f, 0.0f, 0.0f};
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, red);
    glDrawArrays(GL_LINES, 0, 2);

    // Y - G
    float green[3] = {0.0f, 0.3f, 0.0f};
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, green);
    glDrawArrays(GL_LINES, 2, 2);

    // Z - B
    float blue[3] = {0.0f, 0.0f, 0.3f};
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, blue);
    glDrawArrays(GL_LINES, 4, 2);
}