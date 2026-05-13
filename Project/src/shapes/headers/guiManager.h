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
#include "sceneSurface.h"


class GuiManager {
public:
    float min_pos = -50.0f, max_pos = 50.0f;
    float min_scale = 0.01f, max_scale = 10.0f;
    float min_axis = -1.0f, max_axis = 1.0f;
    float min_angle = -360.0f, max_angle = 360.0f;
    float max_R = 10.0f, min_R = 0.1f;
    float max_r = 5.0f, min_r = 0.1f;
    int max_density_R = 80, min_density_R = 3;
    int max_density_r = 80, min_density_r = 3;


    // guiManager.h - dodaj do sekcji public
    int newSurfPatchesU = 1, newSurfPatchesV = 1;
    float newSurfDimU = 5.0f, newSurfDimV = 5.0f;
    int newSurfType = 0; // 0: C0, 1: C2
    bool newSurfIsCylinder = false;
    int surfaceDeletionMode = 2; // 0: Tylko płat, 1: Płat i pkt, 2: Smart

    // guiManager.h
// ... reszta pól ...
    bool isNewSurfacePanelOpen = false; // Do sterowania otwarciem
    bool forceClosePanel = false;       // Flaga do zamknięcia po stworzeniu
    std::shared_ptr<SceneSurface> previewSurface = nullptr;
    std::vector<std::shared_ptr<ScenePoint>> previewPoints; // shared_ptr trzymające punkty przy życiu!

    void refreshPreview(const Cursor& cursor);


    bool isStereoMode = false;
    float eyeSeparation = 0.5f;
    float focalDistance = 20.0f;

    // Stan transformacji z poziomu GUI
    float guiDeltaPos[3] = {0.0f, 0.0f, 0.0f};
    float guiDeltaScale = 1.0f;
    int guiRotMode = 0;
    int prevGuiRotMode = 0;
    float guiRotAxis[3] = {0.0f, 1.0f, 0.0f};
    float guiRotAngle = 0.0f;
    float guiRotQuat[4] = {1.0f, 0.0f, 0.0f, 0.0f};


    bool wasSelectionChanged = false;
    bool wasBaked = false;

    Transformations getGuiDelta() const
    {
        Transformations t;
        t.posX = guiDeltaPos[0];
        t.posY = guiDeltaPos[1];
        t.posZ = guiDeltaPos[2];
        t.scale = guiDeltaScale;

        Quaternion q(1.0f, 0.0f, 0.0f, 0.0f);
        if (guiRotMode == 0)
            q = Quaternion::fromAxisAngle(guiRotAxis[0], guiRotAxis[1], guiRotAxis[2], guiRotAngle * (float)M_PI / 180.0f);
        else if (guiRotMode == 1)
            q = Quaternion(guiRotQuat[0], guiRotQuat[1], guiRotQuat[2], guiRotQuat[3]);
        q.normalize();

        t.rotation = q;
        return t;
    }

    void clearGuiState();


    void Draw(std::vector<std::shared_ptr<SceneObject>>& sceneObjects,
              Cursor& cursor, Camera& camera,
              AppState& state,
              bool isBoxSelecting, double boxStartX, double boxStartY, double boxEndX, double boxEndY,
              bool& magicMode, std::shared_ptr<SceneBezier>& magicCurve,
              bool isCamDragging, Vect3& centerOfSelection);


    void renderObjectGuiRow(std::shared_ptr<SceneObject>& obj, bool& magicMode, std::shared_ptr<SceneBezier>& magicCurve);
    void static createSurfaceLogic(std::vector<std::shared_ptr<SceneObject>>& sceneObjects,
                                   const Cursor& cursor, int patchesU, int patchesV,
                                   float dimU, float dimV, bool isC0, bool isCylinder);
};