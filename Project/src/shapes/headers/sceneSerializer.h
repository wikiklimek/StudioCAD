#pragma once
#include <string>
#include <vector>
#include <memory>
#include "sceneObject.h"

class SceneSerializer {
public:
    // Wczytuje obiekty z JSON i dodaje je do wektora sceny
    static void LoadScene(const std::string& filepath, std::vector<std::shared_ptr<SceneObject>>& sceneObjects);

    // Zapisuje aktualną scenę do JSON (napiszemy to w kolejnym etapie, jak już oswoimy wczytywanie)
    static void SaveScene(const std::string& filepath, const std::vector<std::shared_ptr<SceneObject>>& sceneObjects);
};