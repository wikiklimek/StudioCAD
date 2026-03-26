#pragma once
#include "MG1Math/Vect3.h"
#include <iostream>
#include <math.h>

Vect3 getRayDirection(double mouseX, double mouseY, int winWidth, int winHeight,
                      Vect3 cameraPos, Vect3 target, Vect3 up, float fov);

Vect3 getCursorIntersection(Vect3 rayOrigin, Vect3 rayDir);