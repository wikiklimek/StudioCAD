#pragma once
#include "shader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include "torus.h"
#include "matrixesModelViewProjection.h"

void drawEulerAxes(Shader& shader, unsigned int axisVAO, const float position[3], const float rotations[3], float length);

void drawGlobalAxes(Shader& shader, unsigned int axisVAO, float length);