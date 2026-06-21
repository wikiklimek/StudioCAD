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


    std::weak_ptr<SceneObject> objectA;
    std::weak_ptr<SceneObject> objectB;
    bool wrapUA, wrapVA, wrapUB, wrapVB;


    SceneIntersectionCurve(std::string n, const std::vector<IntersectionPoint>& pts,
                           std::shared_ptr<SceneObject> objA, std::shared_ptr<SceneObject> objB,
                           bool wUA, bool wVA, bool wUB, bool wVB);
    ~SceneIntersectionCurve() override;

    void Init() override;
    void Draw(Shader& shader) override;
    void Draw(Shader& shader, Mat4 parentMatrix) override;


    std::shared_ptr<SceneSplineInterpolating> convertToSpline(std::vector<std::shared_ptr<SceneObject>>& sceneObjects, float tolerance = 0.05f);

private:
    void GenerateParametricTextures();
};