#pragma once
#include "previewFunctions.h"

Vect3 getPreviewPosition(const std::shared_ptr<ScenePoint>& p, const PreviewContext& ctx)
{
    Vect3 pos = p->transformations.getPosition();


    if (!ctx.isTransforming || (!ctx.isEntireScene && !p->isSelected && p->selectedCurvesCount == 0))
        return pos;


    if (ctx.isLocal)
    {
        pos.x += ctx.localDeltaPos.x;
        pos.y += ctx.localDeltaPos.y;
        pos.z += ctx.localDeltaPos.z;
    }
    else
    {
        Vect4 p4(pos.x, pos.y, pos.z, 1.0f);
        Vect4 newP = ctx.groupMat * p4;
        pos = Vect3(newP.x, newP.y, newP.z);
    }

    return pos;
}

void drawObjectWithPreview(const std::shared_ptr<SceneObject>& obj, Shader& shader, const PreviewContext& ctx)
{
    if (obj->objectType == ObjectType::BezierCurveC0)
        return;

    bool isTarget = obj->isSelected;
    if (obj->objectType == ObjectType::Point)
    {
        auto p = std::static_pointer_cast<ScenePoint>(obj);
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
    else
    {
        obj->Draw(shader, ctx.groupMat);
    }
}


PreviewContext buildPreviewContext(const AppState& state,const TransformManager& tm, const GuiManager& gui, Vect3 cursorPosition, Vect3 centerOfSelection)
{
    PreviewContext ctx;
    ctx.isTransforming = (state.inputMode == INPUT_MOUSE && tm.isTransformationActive) || (state.inputMode == INPUT_GUI);
    ctx.isLocal = (state.transformMode == LOCAL);
    ctx.isEntireScene = (state.transformMode == ENTIRE_SCENE);

    if (!ctx.isTransforming) return ctx;


    Transformations activeDelta;
    Vect3 center(0.0);

    if (state.inputMode == INPUT_MOUSE)
    {
        activeDelta = tm.mouseDelta;
        center = tm.centerOfTransformations; // Manager myszki sam pilnuje dobrego środka
    }
    else
    {
        activeDelta = gui.getGuiDelta();
        center = (state.transformMode == ENTIRE_SCENE) ? Vect3(0,0,0) : ((state.transformMode == CURSOR_CENTER) ? cursorPosition : centerOfSelection);
    }


    ctx.localDeltaPos = activeDelta.getPosition();
    ctx.localDeltaScale = activeDelta.scale;
    ctx.localDeltaRot = activeDelta.rotation;


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