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
    if (VAO_vectors) glDeleteVertexArrays(1, &VAO_vectors);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VBO_vectors) glDeleteBuffers(1, &VBO_vectors);
    if (EBO_poly) glDeleteBuffers(1, &EBO_poly);
}

void SceneGregoryPatch::InitBuffers()
{
    if (VAO_surface == 0) glGenVertexArrays(1, &VAO_surface);
    if (VAO_poly == 0) glGenVertexArrays(1, &VAO_poly);
    if (VAO_vectors == 0) glGenVertexArrays(1, &VAO_vectors);

    if (VBO == 0) glGenBuffers(1, &VBO);
    if (VBO_vectors == 0) glGenBuffers(1, &VBO_vectors);
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


void SceneGregoryPatch::DrawVectors(Shader& lineShader, const PreviewContext& ctx)
{
    if (!showVectors || points.size() != 60) return;

    std::vector<Vect3> linesGregory;
    std::vector<Vect3> linesBezier;

    // Nowe bufory na trójkąty tworzące groty strzałek
    std::vector<Vect3> trisGregory;
    std::vector<Vect3> trisBezier;

    // --- LOKALNA FUNKCJA DO GENEROWANIA CZWOROŚCIANU (GROTU) ---
    auto appendArrowhead = [&](std::vector<Vect3>& tris, Vect3 A, Vect3 B) {
        Vect3 D = B - A;
        float len = std::sqrt(D.x*D.x + D.y*D.y + D.z*D.z);
        if (len < 1e-5f) return; // Zabezpieczenie przed dzieleniem przez zero

        Vect3 dir = Vect3(D.x/len, D.y/len, D.z/len);

        // Rozmiar grotu: wysokość i promień podstawy dla czworościanu foremnego
        float h = 0.08f; // Długość grotu (możesz dostosować)
        float r = h / 1.4142f; // Promień wpisujący podstawę dla czworościanu foremnego

        Vect3 C = B - dir * h; // Środek podstawy (cofnięty od czubka B wzdłuż wektora)

        // Szukamy dwóch wektorów prostopadłych U i V do utworzenia płaszczyzny podstawy
        Vect3 up(0.0f, 1.0f, 0.0f);
        // Jeśli wektor jest prawie pionowy, zmieniamy wektor pomocniczy, żeby iloczyn wektorowy nie zniknął
        if (std::abs(dir.y) > 0.99f) up = Vect3(1.0f, 0.0f, 0.0f);

        Vect3 U = Vect3::cross(dir, up).normalize();
        Vect3 V = Vect3::cross(U, dir).normalize();

        // 3 wierzchołki podstawy trójkąta równobocznego (obrót o 0, 120 i 240 stopni)
        // cos(120) = -0.5, sin(120) = sqrt(3)/2 = ~0.866
        Vect3 P0 = C + U * r;
        Vect3 P1 = C + U * (-0.5f * r) + V * (0.866025f * r);
        Vect3 P2 = C + U * (-0.5f * r) + V * (-0.866025f * r);

        // Dodajemy 4 ściany czworościanu (każda to 3 wierzchołki do GL_TRIANGLES)
        // Ściany boczne zbiegające się w czubku 'B'
        tris.push_back(B); tris.push_back(P0); tris.push_back(P1);
        tris.push_back(B); tris.push_back(P1); tris.push_back(P2);
        tris.push_back(B); tris.push_back(P2); tris.push_back(P0);
        // Podstawa zamykająca grot (opcjonalna, ale dobra dla poprawności)
        tris.push_back(P0); tris.push_back(P2); tris.push_back(P1);
    };

    // Pobieramy wektory brzegowe dla każdego z 3 sub-płatów
    for (int p = 0; p < 3; ++p)
    {
        int off = p * 20;

        auto addVectorLine = [&](int edgeIdx, int innerIdx){
            auto p_edge = points[off + edgeIdx].lock();
            auto p_inner = points[off + innerIdx].lock();

            Vect3 pEdge = getPreviewPosition(p_edge, ctx);
            Vect3 pInner = getPreviewPosition(p_inner, ctx);
            Vect3 vectorC1 = pInner - pEdge;

            // Wektor wchodzący w płat Gregory'ego
            Vect3 endGregory = pInner;
            linesGregory.push_back(pEdge);
            linesGregory.push_back(endGregory);
            appendArrowhead(trisGregory, pEdge, endGregory); // Dodaj grot

            // Odbicie wektora w drugą stronę (Bezier)
            Vect3 endBezier = pEdge - vectorC1;
            linesBezier.push_back(pEdge);
            linesBezier.push_back(endBezier);
            appendArrowhead(trisBezier, pEdge, endBezier); // Dodaj grot
        };

        // Zewnętrzna krawędź stykająca się z Bezierem (V=0)
        addVectorLine(1, 6);
        addVectorLine(2, 8);

        // Zewnętrzna krawędź stykająca się z Bezierem (U=0)
        addVectorLine(4, 5);
        addVectorLine(10, 11);
    }

    lineShader.use();
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(lineShader.ID, "model"), 1, GL_FALSE, id.table);

    glBindVertexArray(VAO_vectors);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_vectors);

    float yellow[3] = {1.0f, 1.0f, 0.0f};
    float cyjan[3] = {0.0f, 1.0f, 1.0f};

    // ==========================================================
    // 1. RYSOWANIE LINII (Oś wektora)
    // ==========================================================

    // Linie Gregory'ego
    glBufferData(GL_ARRAY_BUFFER, linesGregory.size() * sizeof(Vect3), linesGregory.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, yellow);
    glDrawArrays(GL_LINES, 0, linesGregory.size());

    // Linie Beziera
    glBufferData(GL_ARRAY_BUFFER, linesBezier.size() * sizeof(Vect3), linesBezier.data(), GL_DYNAMIC_DRAW);
    glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, cyjan);
    glDrawArrays(GL_LINES, 0, linesBezier.size());

    // ==========================================================
    // 2. RYSOWANIE GROTÓW (Zamalowane Czworościany + Obramowanie)
    // ==========================================================

    float black[3] = {0.0f, 0.0f, 0.0f}; // Kolor obramowania (czarny)

    // Włączamy przesunięcie głębi, aby zamalowane trójkąty nie "szarpały" się
    // (tzw. Z-fighting) z krawędziami, które narysujemy dokładnie w tym samym miejscu.
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);

    // Groty Gregory'ego
    if (!trisGregory.empty()) {
        glBufferData(GL_ARRAY_BUFFER, trisGregory.size() * sizeof(Vect3), trisGregory.data(), GL_DYNAMIC_DRAW);

        // Krok A: Rysujemy wypełnienie (żółte z nałożonym offsetem)
        glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, yellow);
        glDrawArrays(GL_TRIANGLES, 0, trisGregory.size());

        // Krok B: Rysujemy obramowanie (czarne krawędzie)
        glDisable(GL_POLYGON_OFFSET_FILL);         // Wyłączamy offset dla linii
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Przełączamy OpenGL w tryb "Wireframe"

        glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, black);
        glDrawArrays(GL_TRIANGLES, 0, trisGregory.size());

        // Przywracamy stany do kolejnego rysowania (dla grotów Beziera)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_POLYGON_OFFSET_FILL);
    }

    // Groty Beziera
    if (!trisBezier.empty()) {
        glBufferData(GL_ARRAY_BUFFER, trisBezier.size() * sizeof(Vect3), trisBezier.data(), GL_DYNAMIC_DRAW);

        // Krok A: Rysujemy wypełnienie (cyjanowe z nałożonym offsetem)
        glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, cyjan);
        glDrawArrays(GL_TRIANGLES, 0, trisBezier.size());

        // Krok B: Rysujemy obramowanie (czarne krawędzie)
        glDisable(GL_POLYGON_OFFSET_FILL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glUniform3fv(glGetUniformLocation(lineShader.ID, "objectColor"), 1, black);
        glDrawArrays(GL_TRIANGLES, 0, trisBezier.size());

        // Przywracamy stany do domyślnych (żeby nie popsuć renderowania reszty sceny)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Sprzątanie maszyny stanów po sobie
    glDisable(GL_POLYGON_OFFSET_FILL);
}


void SceneGregoryPatch::Init()
{
    InitBuffers();
    InitPolygonIndices();

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
