#pragma once
#include <MG1Math/Vect3.h>
#include <MG1Math/Vect4.h>
#include <MG1Math/Mat3.h>
#include <MG1Math/Mat4.h>

// Przykładowe nagłówki funkcji (możesz użyć swojej biblioteki MG1Math)
Mat4 createModelMatrix(Vect3 position, Vect3 rotation, float scale);
Mat4 createProjectionMatrix(float fov, float aspect, float n, float f);
Mat4 createViewMatrix(Vect3 cameraPos, Vect3 target, Vect3 up);