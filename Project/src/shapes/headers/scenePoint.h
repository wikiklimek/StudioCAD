#pragma once
#include "sceneObject.h"
#include "MG1Math/Mat4.h"
#include <vector>

class ScenePoint : public SceneObject {
public:
    int selectedCurvesCount = 0;
    unsigned int VAO, VBO;
    float size;

    bool isVirtual = false;
    bool wasGuiEdited = false;

    bool isSelectedAsDeBoore = false;
    float virtualWeight = 0.0f;


    // NOWE FLAGI
    bool belongsToPatch = false;      // Blokada usuwania (wymóg zadania 7)
    bool isSelectedViaPatch = false; // Czy płat, do którego należę, jest wybrany
    int globalCurvesCount = 0;       // Całkowita liczba struktur używających tego punktu

    // Metoda pomocnicza dla GUI
    bool canBeDeleted() const {
        return !belongsToPatch;
    }

    bool isAnyWaySelected();


    ScenePoint(std::string n, Transformations spawnTransform);
    ScenePoint(std::string n, float s, Transformations spawnTransform);
    ~ScenePoint() override;

    void Init() override;
    void Draw(Shader& shader) override;
    void Draw(Shader& shader, Mat4 parentMatrix) override;
    void DrawAtPosition(Shader& shader, Vect3 Position);



};