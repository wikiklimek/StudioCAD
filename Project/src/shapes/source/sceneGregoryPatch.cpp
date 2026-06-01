#include "sceneGregoryPatch.h"
#include "previewFunctions.h"
#include <glad/glad.h>
#include <GL/gl.h>

SceneGregoryPatch::SceneGregoryPatch(std::string n, Transformations spawnTransform)
        : SceneObject(std::move(n), spawnTransform, ObjectType::GregoryPatch)
{
}


SceneGregoryPatch::~SceneGregoryPatch()
{
    if (VAO_surface) glDeleteVertexArrays(1, &VAO_surface);
    if (VAO_poly) glDeleteVertexArrays(1, &VAO_poly);
    if (VAO_vectors) glDeleteVertexArrays(1, &VAO_vectors); // Czyszczenie
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VBO_vectors) glDeleteBuffers(1, &VBO_vectors);       // Czyszczenie
    if (EBO_poly) glDeleteBuffers(1, &EBO_poly);
}

void SceneGregoryPatch::InitBuffers()
{
    if (VAO_surface == 0) glGenVertexArrays(1, &VAO_surface);
    if (VAO_poly == 0) glGenVertexArrays(1, &VAO_poly);
    if (VAO_vectors == 0) glGenVertexArrays(1, &VAO_vectors); // Inicjalizacja

    if (VBO == 0) glGenBuffers(1, &VBO);
    if (VBO_vectors == 0) glGenBuffers(1, &VBO_vectors);       // Inicjalizacja
    if (EBO_poly == 0) glGenBuffers(1, &EBO_poly);
}

void SceneGregoryPatch::InitPolygonIndices()
{
    polyIndices.clear();

    for (int p = 0; p < 3; ++p)
    {
        int offset = p * 20;

        // 1. OBWIEDNIA ZEWNĘTRZNA (Ramka sub-płata)
        // Górna
        polyIndices.push_back(offset + 0); polyIndices.push_back(offset + 1);
        polyIndices.push_back(offset + 1); polyIndices.push_back(offset + 2);
        polyIndices.push_back(offset + 2); polyIndices.push_back(offset + 3);
        // Prawa
        polyIndices.push_back(offset + 3); polyIndices.push_back(offset + 9);
        polyIndices.push_back(offset + 9); polyIndices.push_back(offset + 15);
        polyIndices.push_back(offset + 15); polyIndices.push_back(offset + 19);
        // Dolna
        polyIndices.push_back(offset + 19); polyIndices.push_back(offset + 18);
        polyIndices.push_back(offset + 18); polyIndices.push_back(offset + 17);
        polyIndices.push_back(offset + 17); polyIndices.push_back(offset + 16);
        // Lewa
        polyIndices.push_back(offset + 16); polyIndices.push_back(offset + 10);
        polyIndices.push_back(offset + 10); polyIndices.push_back(offset + 4);
        polyIndices.push_back(offset + 4); polyIndices.push_back(offset + 0);

        // 2. LINIE WEWNĘTRZNE (Powiązania do punktów C1 i zawiasów)
        // Krawędź V=0 (dolna) w głąb do p11v i p12v
        polyIndices.push_back(offset + 1); polyIndices.push_back(offset + 6);  // P01 -> p11v
        polyIndices.push_back(offset + 2); polyIndices.push_back(offset + 8);  // P02 -> p12v

        // Krawędź U=0 (lewa) w głąb do p11u i p21u
        polyIndices.push_back(offset + 4); polyIndices.push_back(offset + 5);  // P10 -> p11u
        polyIndices.push_back(offset + 10); polyIndices.push_back(offset + 11); // P20 -> p21u

        // Krawędź V=1 (promienista od środka) w głąb do p21v i p22v
        polyIndices.push_back(offset + 17); polyIndices.push_back(offset + 12); // P31 -> p21v
        polyIndices.push_back(offset + 18); polyIndices.push_back(offset + 14); // P32 -> p22v

        // Krawędź U=1 (promienista od środka) w głąb do p12u i p22u
        polyIndices.push_back(offset + 9); polyIndices.push_back(offset + 7);   // P13 -> p12u
        polyIndices.push_back(offset + 15); polyIndices.push_back(offset + 13); // P23 -> p22u
    }

    glBindVertexArray(VAO_poly);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // VBO współdzielone z punktami powierzchni
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_poly);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, polyIndices.size() * sizeof(unsigned int), polyIndices.data(), GL_STATIC_DRAW);
}

// DrawPolygon zostaje uproszczone, bo wektory wynosimy do osobnej funkcji:
void SceneGregoryPatch::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    if (!showPolygon || points.size() != 60 || polyIndices.empty()) return;

    lineShader.use();
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "model"), 1, GL_FALSE, id.table);

    float gray[3] = {0.4f, 0.4f, 0.4f};
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, gray);
    glBindVertexArray(VAO_poly);
    glDrawElements(GL_LINES, polyIndices.size(), GL_UNSIGNED_INT, 0);
}

// NOWA FUNKCJA DO RYSOWANIA WEKTORÓW C1 NA BRZEGU OTWORU
// NOWA FUNKCJA DO RYSOWANIA WEKTORÓW C1 NA BRZEGU OTWORU
void SceneGregoryPatch::DrawVectors(Shader& lineShader, const PreviewContext& ctx)
{
    if (!showVectors || points.size() != 60) return;

    std::vector<Vect3> linesGregory;
    std::vector<Vect3> linesBezier;

    // Pobieramy wektory brzegowe dla każdego z 3 sub-płatów
    for (int p = 0; p < 3; ++p)
    {
        int off = p * 20;

        // Lambda pomocnicza do tworzenia par wierzchołków dla pojedynczego wektora
        auto addVectorLine = [&](int edgeIdx, int innerIdx){

            auto p_edge = points[off + edgeIdx].lock();
            auto p_inner = points[off + innerIdx].lock();

            Vect3 pEdge = getPreviewPosition(p_edge, ctx);
            Vect3 pInner = getPreviewPosition(p_inner, ctx);
            Vect3 vectorC1 = pInner - pEdge; // To jest nasza pochodna (Vektor C1)

            // Wektor wchodzący w płat Gregory'ego
            linesGregory.push_back(pEdge);
            linesGregory.push_back(pInner);

            // Odbicie wektora w drugą stronę (pokazuje, gdzie matematycznie "patrzy" Bezier)
            linesBezier.push_back(pEdge);
            linesBezier.push_back(pEdge - vectorC1);
        };

        // Zewnętrzna krawędź stykająca się z Bezierem z jednej strony (V=0)
        addVectorLine(1, 6); // Rysuje wektor z punktu P01 do p11v
        addVectorLine(2, 8); // Rysuje wektor z punktu P02 do p12v

        // Zewnętrzna krawędź stykająca się z Bezierem z drugiej strony (U=0)
        addVectorLine(4, 5); // Rysuje wektor z punktu P10 do p11u
        addVectorLine(10, 11); // Rysuje wektor z punktu P20 do p21u
    }

    lineShader.use();
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "model"), 1, GL_FALSE, id.table);

    glBindVertexArray(VAO_vectors);

    // ========================================================
    // KRYTYCZNA LINIJKA - chronimy główny VBO powierzchni!
    // ========================================================
    glBindBuffer(GL_ARRAY_BUFFER, VBO_vectors);

    // 1. Rysujemy wektory Gregory'ego (Żółte)
    glBufferData(GL_ARRAY_BUFFER, linesGregory.size() * sizeof(Vect3), linesGregory.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);

    float yellow[3] = {1.0f, 1.0f, 0.0f}; // Poprawiony żółty
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, yellow);
    glDrawArrays(GL_LINES, 0, linesGregory.size());

    // 2. Rysujemy przeciwne wektory pokazujące ciągłość z Bezierem (Cyjanowe)
    glBufferData(GL_ARRAY_BUFFER, linesBezier.size() * sizeof(Vect3), linesBezier.data(), GL_DYNAMIC_DRAW);
    // Nie musimy znowu ustawiać glVertexAttribPointer, bo bufor i atrybut są te same

    float cyjan[3] = {0.0f, 1.0f, 1.0f};
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, cyjan);
    glDrawArrays(GL_LINES, 0, linesBezier.size());
}


void SceneGregoryPatch::Init()
{
    InitBuffers();
    InitPolygonIndices();

    // ========================================================
    // KRYTYCZNA POPRAWKA: Konfiguracja VAO dla powierzchni!
    // ========================================================
    glBindVertexArray(VAO_surface);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Mówimy GPU: "Bierz 3 floaty z bufora jako 1 wierzchołek"
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void SceneGregoryPatch::DrawSurface(Shader& shader, const PreviewContext& ctx)
{
    // Zabezpieczenie: Płat Gregory'ego musi mieć dokładnie 60 punktów (3x20)!
    if (points.size() != 60) return;

    // 1. Aktualizacja pozycji z uwzględnieniem podglądu (Live Preview)
    std::vector<Vect3> currentPositions;
    currentPositions.reserve(60);
    for (auto& wp : points)
    {
        if (auto p = wp.lock())
            currentPositions.push_back(getPreviewPosition(p, ctx));
        else
            currentPositions.push_back(Vect3(0.0f)); // Zabezpieczenie przed usuniętym punktem
    }

    bool needsUpload = false;
    if (currentPositions.size() != prevPositions.size()) {
        needsUpload = true;
    } else {
        for (size_t i = 0; i < currentPositions.size(); ++i) {
            if (currentPositions[i].x != prevPositions[i].x ||
                currentPositions[i].y != prevPositions[i].y ||
                currentPositions[i].z != prevPositions[i].z)
            {
                needsUpload = true;
                break;
            }
        }
    }

    if (needsUpload)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, currentPositions.size() * sizeof(Vect3), currentPositions.data(), GL_DYNAMIC_DRAW);
        prevPositions = currentPositions;
    }

    // 2. Przygotowanie Shadera
    shader.use();
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, id.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDraw());

    glBindVertexArray(VAO_surface);

    // KRYTYCZNE: Informujemy GPU, że jedna paczka ma 20 wierzchołków!
    glPatchParameteri(GL_PATCH_VERTICES, 20);

    // 3. Rysowanie 3 sub-płatów, każdy z WŁASNYM samplingiem U i V
    for (int i = 0; i < 3; ++i)
    {
        glUniform1i(glGetUniformLocation(shader.ID, "u_tessLevelU"), samplesU[i]);
        glUniform1i(glGetUniformLocation(shader.ID, "u_tessLevelV"), samplesV[i]);

        // Rysowanie linii poziomych
        glUniform1i(glGetUniformLocation(shader.ID, "swapUV"), 0);
        glDrawArrays(GL_PATCHES, i * 20, 20);

        // Rysowanie linii pionowych
        glUniform1i(glGetUniformLocation(shader.ID, "swapUV"), 1);
        glDrawArrays(GL_PATCHES, i * 20, 20);
    }
}
