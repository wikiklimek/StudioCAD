#pragma once
#include "sceneObject.h"
#include "intersectionSolver.h"
#include "sceneSplineInterpolating.h"
#include "scenePoint.h"
#include <vector>
#include <memory>
#include <glad/glad.h>

class SceneIntersectionCurve : public SceneObject {
public:
    std::vector<IntersectionPoint> intersectionPoints;
    unsigned int VAO = 0, VBO = 0;

    GLuint textureA = 0;
    GLuint textureB = 0;
    bool showTextureA = false;
    bool showTextureB = false;

    SceneIntersectionCurve(std::string n, const std::vector<IntersectionPoint>& pts);
    ~SceneIntersectionCurve() override;

    void Init() override;
    void Draw(Shader& shader) override;
    void Draw(Shader& shader, Mat4 parentMatrix) override;

    // Wymóg: konwersja krzywej numerycznej do krzywej sklejanej C2
    std::shared_ptr<SceneSplineInterpolating> convertToSpline(std::vector<std::shared_ptr<SceneObject>>& sceneObjects, int pointStep = 10);

private:
    void GenerateParametricTextures();
};