#pragma once
#include "previewContext.h"
#include "transformManager.h"
#include "guiManager.h"
#include <vector>
#include <memory>

Vect3 getPreviewPosition(const std::shared_ptr<ScenePoint>& p, const PreviewContext& ctx);
void drawObjectWithPreview(const std::shared_ptr<SceneObject>& obj, Shader& shader, const PreviewContext& ctx);

PreviewContext buildPreviewContext(const AppState& state,const TransformManager& tm, const GuiManager& gui, Vect3 cursorPosition, Vect3 centerOfSelection);