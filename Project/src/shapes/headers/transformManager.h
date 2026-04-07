#pragma once


#include <memory>
#include <vector>
#include "camera.h"
#include "transformations.h"
#include "sceneObject.h"

Vect3 calculateScreenSpaceTranslation(float dx_world, float dy_world, Vect3 centerOfTransformations, const Camera& camera);

class TransformManager {
public:
    TransformMode transformMode = LOCAL;
    InputMode inputMode = INPUT_MOUSE;
    InputMode prevInputMode = INPUT_MOUSE;
    DragMode currentMode = BOX;

    bool isTransformationActive = false;
    Transformations mouseDelta;
    Vect3 centerOfTransformations = Vect3(0.0f);

    // Wywoływane, gdy klikniesz na scenie, żeby zacząć ciągnąć
    void startTransformation(Vect3 centerOfSelection, Vect3 cursorPosition);

    // Matematyka myszki w jednym miejscu!
    void processMouseDrag(float dx_world, float dy_world, const Camera& camera);


    // Wypiekanie myszki w jednym miejscu!
    void bakeMouseTransformations(std::vector<std::shared_ptr<SceneObject>>& sceneObjects);
};