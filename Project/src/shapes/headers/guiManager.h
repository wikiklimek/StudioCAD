#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include <algorithm>
#include "cursor.h"
#include "camera.h"
#include "sceneBezierC0.h"
#include "transformManager.h"


class GuiManager {
public:
    // Limity sliderów z głównego programu
    float min_pos = -50.0f, max_pos = 50.0f;
    float min_scale = 0.01f, max_scale = 10.0f;
    float min_axis = -1.0f, max_axis = 1.0f;
    float min_angle = -360.0f, max_angle = 360.0f;
    float max_R = 10.0f, min_R = 0.1f;
    float max_r = 5.0f, min_r = 0.1f;
    int max_density_R = 80, min_density_R = 3;
    int max_density_r = 80, min_density_r = 3;

    // Stan transformacji z poziomu GUI
    float guiDeltaPos[3] = {0.0f, 0.0f, 0.0f};
    float guiDeltaScale = 1.0f;
    int guiRotMode = 0;
    int prevGuiRotMode = 0;
    float guiRotAxis[3] = {0.0f, 1.0f, 0.0f};
    float guiRotAngle = 0.0f;
    float guiRotQuat[4] = {1.0f, 0.0f, 0.0f, 0.0f};

    void clearGuiState();

    // PODMIENIONY NAGŁÓWEK FUNKCJI:
    void Draw(std::vector<std::shared_ptr<SceneObject>>& sceneObjects,
              Cursor& cursor, Camera& camera,
              TransformManager& tm, // <---- WSZYSTKIE 3 ENUMY ZASTĄPIONE TYM!
              bool isBoxSelecting, double boxStartX, double boxStartY, double boxEndX, double boxEndY,
              bool& magicMode, std::shared_ptr<SceneBezierC0>& magicCurve,
              bool isCamDragging, Vect3& centerOfSelection);


    void renderObjectGuiRow(std::shared_ptr<SceneObject>& obj, bool& magicMode, std::shared_ptr<SceneBezierC0>& magicCurve) const;

};