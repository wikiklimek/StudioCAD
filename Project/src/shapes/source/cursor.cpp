#include "cursor.h"
#include <vector>
#include <cmath>

Cursor::~Cursor()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
}

void Cursor::Init()
{
    std::vector<Vect3> vertices;

    // linie (Indeksy 0 - 5)
    vertices.push_back(Vect3(0.0f, 0.0f, 0.0f)); vertices.push_back(Vect3(1.0f, 0.0f, 0.0f)); // X
    vertices.push_back(Vect3(0.0f, 0.0f, 0.0f)); vertices.push_back(Vect3(0.0f, 1.0f, 0.0f)); // Y
    vertices.push_back(Vect3(0.0f, 0.0f, 0.0f)); vertices.push_back(Vect3(0.0f, 0.0f, 1.0f)); // Z

    // generowanie gortów
    auto appendArrowhead = [&](Vect3 A, Vect3 B) {
        Vect3 D = B - A;
        Vect3 dir = D.normalize();

        float h = 0.2f; // skala rozmiaru grota
        float r = h / 1.4142f;

        Vect3 C = B - dir * h;

        Vect3 up(0.0f, 1.0f, 0.0f);
        if (std::abs(dir.y) > 0.99f) up = Vect3(1.0f, 0.0f, 0.0f);

        Vect3 U = Vect3::cross(dir, up).normalize();
        Vect3 V = Vect3::cross(U, dir).normalize();

        Vect3 P0 = C + U * r;
        Vect3 P1 = C + U * (-0.5f * r) + V * (0.866025f * r);
        Vect3 P2 = C + U * (-0.5f * r) + V * (-0.866025f * r);


        vertices.push_back(B);
        vertices.push_back(P0);
        vertices.push_back(P1);

        vertices.push_back(B);
        vertices.push_back(P1);
        vertices.push_back(P2);

        vertices.push_back(B);
        vertices.push_back(P2);
        vertices.push_back(P0);

        vertices.push_back(P0);
        vertices.push_back(P2);
        vertices.push_back(P1);
    };

    // groty (Indeksy 6 - 41)
    appendArrowhead(Vect3(0.0f, 0.0f, 0.0f), Vect3(1.0f, 0.0f, 0.0f)); // Grot X
    appendArrowhead(Vect3(0.0f, 0.0f, 0.0f), Vect3(0.0f, 1.0f, 0.0f)); // Grot Y
    appendArrowhead(Vect3(0.0f, 0.0f, 0.0f), Vect3(0.0f, 0.0f, 1.0f)); // Grot Z

    // Wgrywamy całość (linie + trójkąty) do pamięci karty graficznej RAZ na start (STATIC_DRAW)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vect3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);
}


void Cursor::Draw(Shader& shader)
{
    Mat4 modelMat = createModelMatrix(transform.getPosition(), transform.rotation, transform.scale);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, modelMat.table);
    glBindVertexArray(VAO);

    float colorX[3] = { 1.0f, 0.0f, 0.0f };
    float colorY[3] = { 0.0f, 1.0f, 0.0f };
    float colorZ[3] = { 0.1f, 0.5f, 1.0f };
    float black[3]  = { 0.0f, 0.0f, 0.0f };

    // rysowanie linnii w roznych kolorach
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, colorX);
    glDrawArrays(GL_LINES, 0, 2); // Linia X

    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, colorY);
    glDrawArrays(GL_LINES, 2, 2); // Linia Y

    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, colorZ);
    glDrawArrays(GL_LINES, 4, 2); // Linia Z

    // rysowanie gortów
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, colorX);
    glDrawArrays(GL_TRIANGLES, 6, 12);  // Grot X (indeksy 6-17)

    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, colorY);
    glDrawArrays(GL_TRIANGLES, 18, 12); // Grot Y (indeksy 18-29)

    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, colorZ);
    glDrawArrays(GL_TRIANGLES, 30, 12); // Grot Z (indeksy 30-41)

    //rysowanie robramowania grotów
    glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, black);
    // Skoro wszystkie 3 groty to po prostu 36 wierzchołków pod rząd,
    // możemy narysować obramowanie dla wszystkich naraz jednym wywołaniem!
    glDrawArrays(GL_TRIANGLES, 6, 36);

    // Reset ustawień OpenGL
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}