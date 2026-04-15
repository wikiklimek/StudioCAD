#pragma once
enum DragMode { BOX, TRANSLATE, ROTATE_X, ROTATE_Y, ROTATE_Z, SCALE, ROTATE_FREE };
enum CameraDragMode { CAM_NONE, CAM_ORBIT, CAM_ZOOM, CAM_PAN };


enum TransformMode { LOCAL, COMMON_CENTER, CURSOR_CENTER, ENTIRE_SCENE };
enum InputMode { INPUT_MOUSE, INPUT_GUI };

enum class ObjectType {
    Point,
    Torus,
    BezierCurveC0
};

enum BezierDrawMode {GEOMETRY, LINE_STRIP};