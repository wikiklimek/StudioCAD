#pragma once
#include <MG1Math/Vect3.h>
#include <MG1Math/Vect4.h>
#include <MG1Math/Mat3.h>
#include <MG1Math/Mat4.h>
#include "quaternion.h"


Mat4 createModelMatrix(Vect3 position, Quaternion rotationMat, float scale);
Mat4 createProjectionMatrix(float fov, float aspect, float n, float f);
Mat4 createViewMatrix(Vect3 cameraPos, Vect3 target, Vect3 up);

Mat4 createFrustum(float l, float r, float b, float t, float n, float f);
void getStereoMatrices(float fov, float aspect, float n, float f, float eyeSeparation,
                       float zpd, bool isLeftEye, Mat4& outProj, Mat4& outViewShift);

