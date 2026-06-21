#pragma once
#include "previewFunctions.h"

Vect3 getPreviewPosition(const std::shared_ptr<ScenePoint>& p, const PreviewContext& ctx)
{
    Vect3 pos = p->transformations.getPosition();


    if (!ctx.isTransforming || (!p->isSelected &&
                                p->selectedCurvesCount == 0 &&
                                !p->isSelectedAsDeBoore &&
                                p->selectedSurfacesCount == 0))
        return pos;

    Vect3 delta(0.0f);
    if (ctx.isLocal)
    {
        delta = ctx.localDeltaPos; //bo pozycja w lokalnym układzie nie ma obrotu/skali istotnej
    }
    else
    {
        Vect4 p4(pos.x, pos.y, pos.z, 1.0f);
        delta = (ctx.groupMat * p4).toVect3() - pos;
    }

    if (p->isSelectedAsDeBoore)
    {
        return pos + delta * p->virtualWeight;
    }

    return pos + delta;
}

void drawObjectWithPreview(const std::shared_ptr<SceneObject>& obj, Shader& shader, const PreviewContext& ctx)
{
    shader.use();
    //tutaj bedziemy robili draw:
    //      BezierC2
    //      SplineInterpolatibg
    //      BezierSurfaceC2
    // bo rysujemy duszki punktów beziera
    if (obj->objectType == ObjectType::BezierCurveC0 || obj->objectType == ObjectType::BezierSurfaceC0)
        return;

    bool isTarget = obj->isSelected;
    bool asDeBoore = false;

    if (obj->objectType == ObjectType::Point)
    {
        auto p = std::static_pointer_cast<ScenePoint>(obj);
        if (p->selectedCurvesCount > 0 || p->selectedSurfacesCount > 0)
            isTarget = true;
        if (p->isSelectedAsDeBoore)
            asDeBoore = true;
    }

    if (!ctx.isTransforming || (!isTarget && !asDeBoore))
    {
        obj->Draw(shader);
        return;
    }


    if(asDeBoore && obj->objectType == ObjectType::Point)
    {
        auto p = std::static_pointer_cast<ScenePoint>(obj);
        p->DrawAtPosition(shader, getPreviewPosition(p, ctx));
        return;
    }


    //Aplikowanie i cofanie zmian dla Trybu Lokalnego
    if (ctx.isLocal)
    {
        Transformations old = obj->transformations;

        obj->transformations.posX += ctx.localDeltaPos.x;
        obj->transformations.posY += ctx.localDeltaPos.y;
        obj->transformations.posZ += ctx.localDeltaPos.z;
        obj->transformations.scale *= ctx.localDeltaScale;
        obj->transformations.rotation = ctx.localDeltaRot * obj->transformations.rotation;
        obj->transformations.rotation.normalize();

        obj->Draw(shader);

        // Cofamy zmiany po rysowaniu
        obj->transformations = old;
    }
    else
    {
        obj->Draw(shader, ctx.groupMat);
    }
}


PreviewContext buildPreviewContext(const AppState& state,const TransformManager& tm,
                                   const GuiManager& gui, Vect3 cursorPosition,
                                   Vect3 centerOfSelection)
{
    PreviewContext ctx;
    ctx.isTransforming = (state.inputMode == INPUT_MOUSE && tm.isTransformationActive) || (state.inputMode == INPUT_GUI);
    ctx.isLocal = (state.transformMode == LOCAL);

    ctx.anySelectionChanged = gui.wasSelectionChanged || tm.wasSelectionChanged;
    ctx.wasBaked = gui.wasBaked || tm.wasBaked;

    if (!ctx.isTransforming)
        return ctx;


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
        center = ((state.transformMode == CURSOR_CENTER) ? cursorPosition : centerOfSelection);
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