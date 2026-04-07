#pragma once

#include <memory>
#include <vector>
#include "camera.h"
#include "transformations.h"
#include "sceneObject.h"
#include "appState.h" // <--- DODANE

Vect3 calculateScreenSpaceTranslation(float dx_world, float dy_world, Vect3 centerOfTransformations, const Camera& camera);

class TransformManager {
public:
    bool isTransformationActive = false;
    Transformations mouseDelta;
    Vect3 centerOfTransformations = Vect3(0.0f);

    // Przekazujemy AppState, żeby wiedział, jaki środek ustawić
    void startTransformation(Vect3 centerOfSelection, Vect3 cursorPosition, const AppState& state);

    // Przekazujemy AppState, żeby wiedział, jak liczyć (Translate czy Rotate?)
    void processMouseDrag(float dx_world, float dy_world, const Camera& camera, const AppState& state);

    // Przekazujemy AppState, żeby wiedział, jak wypiekać (Local czy Group?)
    void bakeMouseTransformations(std::vector<std::shared_ptr<SceneObject>>& sceneObjects, const AppState& state);
};