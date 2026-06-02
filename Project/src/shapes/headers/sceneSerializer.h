#pragma once
#include <string>
#include <vector>
#include <memory>
#include "sceneObject.h"

class SceneSerializer {
public:
    static void LoadScene(const std::string& filepath, std::vector<std::shared_ptr<SceneObject>>& sceneObjects);
    static void SaveScene(const std::string& filepath, const std::vector<std::shared_ptr<SceneObject>>& sceneObjects);
};