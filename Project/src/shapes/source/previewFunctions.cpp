#pragma once
#include "previewFunctions.h"

void drawObjectWithPreview(const std::shared_ptr<SceneObject>& obj, Shader& shader, const PreviewContext& ctx)
{
    if (obj->objectType == ObjectType::BezierCurveC0)
        return;

    bool isTarget = obj->isSelected;
    if (auto p = std::dynamic_pointer_cast<ScenePoint>(obj))
    {
        if (p->selectedCurvesCount > 0)
            isTarget = true;
    }
    if (ctx.isEntireScene)
        isTarget = true;


    if (!ctx.isTransforming || !isTarget)
    {
        obj->Draw(shader);
        return;
    }

    //Aplikowanie i cofanie zmian dla Trybu Lokalnego
    if (ctx.isLocal)
    {
        Quaternion oldRot = obj->transformations.rotation;

        obj->transformations.posX += ctx.localDeltaPos.x;
        obj->transformations.posY += ctx.localDeltaPos.y;
        obj->transformations.posZ += ctx.localDeltaPos.z;
        obj->transformations.scale *= ctx.localDeltaScale;
        obj->transformations.rotation = ctx.localDeltaRot * obj->transformations.rotation;
        obj->transformations.rotation.normalize();

        obj->Draw(shader);

        // Cofamy zmiany po rysowaniu
        obj->transformations.posX -= ctx.localDeltaPos.x;
        obj->transformations.posY -= ctx.localDeltaPos.y;
        obj->transformations.posZ -= ctx.localDeltaPos.z;
        obj->transformations.scale /= ctx.localDeltaScale;
        obj->transformations.rotation = oldRot;
    }
        // 3. Aplikowanie zmian dla Trybu Grupowego (używamy przeciążenia Draw(shader, mat4))
    else
    {
        obj->Draw(shader, ctx.groupMat);
    }
}


PreviewContext buildPreviewContext(const TransformManager& tm, const GuiManager& gui, Vect3 cursorPosition, Vect3 centerOfSelection)
{
    PreviewContext ctx;
    ctx.isTransforming = (tm.inputMode == INPUT_MOUSE && tm.isTransformationActive) || (tm.inputMode == INPUT_GUI);
    ctx.isLocal = (tm.transformMode == LOCAL);
    ctx.isEntireScene = (tm.transformMode == ENTIRE_SCENE);

    if (!ctx.isTransforming) return ctx;

    // Magia Unifikacji - jedna zmienna dla obu wejść!
    Transformations activeDelta;
    Vect3 center(0.0);

    if (tm.inputMode == INPUT_MOUSE)
    {
        activeDelta = tm.mouseDelta;
        center = tm.centerOfTransformations; // Manager myszki sam pilnuje dobrego środka
    }
    else
    {
        activeDelta = gui.getGuiDelta();     // Manager GUI zwraca ustandaryzowaną paczkę
        center = (tm.transformMode == ENTIRE_SCENE) ? Vect3(0,0,0) : ((tm.transformMode == CURSOR_CENTER) ? cursorPosition : centerOfSelection);
    }

    // Wspólne przypisanie
    ctx.localDeltaPos = activeDelta.getPosition();
    ctx.localDeltaScale = activeDelta.scale;
    ctx.localDeltaRot = activeDelta.rotation;

    // Wyliczenie macierzy grupowej tylko raz dla wszystkich!
    if (!ctx.isLocal)
    {
        Mat4 T_toOrigin = Mat4::translate_inverse(center);
        Mat4 R_group = ctx.localDeltaRot.toMat4();
        Mat4 S_group = Mat4::scale(Vect3(ctx.localDeltaScale, ctx.localDeltaScale, ctx.localDeltaScale));
        Mat4 T_toPos = Mat4::translate(center + ctx.localDeltaPos);
        ctx.groupMat = T_toPos * R_group * S_group * T_toOrigin;
    }

    return ctx;
}