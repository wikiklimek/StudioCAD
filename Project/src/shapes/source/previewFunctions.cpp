#pragma once
#include "previewFunctions.h"

Vect3 getPreviewPosition(const std::shared_ptr<ScenePoint>& p, const PreviewContext& ctx)
{
    Vect3 pos = p->transformations.getPosition();


    if (!ctx.isTransforming || (!p->isSelected && p->selectedCurvesCount == 0 && !p->isSelectedAsDeBoore))
        return pos;

    Vect3 delta(0.0f);
    if (ctx.isLocal)
    {
        delta = ctx.localDeltaPos;
    }
    else
    {
        Vect4 p4(pos.x, pos.y, pos.z, 1.0f);
        delta = (ctx.groupMat * p4).toVect3() - pos;
    }

    // TWOJA LOGIKA: Jeśli poruszany pośrednio przez Bernsteina, nakładamy wagę (np. 1.5f)!
    if (p->isSelectedAsDeBoore)
    {
        return pos + delta * p->virtualWeight;
    }

    return pos + delta;
}

void drawObjectWithPreview(const std::shared_ptr<SceneObject>& obj, Shader& shader, const PreviewContext& ctx)
{
    //tutaj bedziemy robili draw BezierC2 bo rysujemy duszki punktów beziera
    if (obj->objectType == ObjectType::BezierCurveC0) return;

    bool isTarget = obj->isSelected;
    bool asDeBoore = false;

    if (obj->objectType == ObjectType::Point)
    {
        auto p = std::static_pointer_cast<ScenePoint>(obj);
        if (p->selectedCurvesCount > 0)
            isTarget = true;
        if (p->isSelectedAsDeBoore)
            asDeBoore = true;
    }

    if (!ctx.isTransforming || (!isTarget && !asDeBoore))
    {
        obj->Draw(shader);
        return;
    }

    // RYSOWANIE KWADRACIKA W LOCIE (Dla pośrednio ciągniętych De Boorów)
    if (asDeBoore) {
        auto p = std::static_pointer_cast<ScenePoint>(obj);
        Vect3 oldPos = p->transformations.getPosition();
        Vect3 delta(0.0f);
        if (ctx.isLocal) delta = ctx.localDeltaPos;
        else { Vect4 p4(oldPos.x, oldPos.y, oldPos.z, 1.0f); delta = (ctx.groupMat * p4).toVect3() - oldPos; }

        p->transformations.setPosition(oldPos + delta * p->virtualWeight);
        p->Draw(shader);
        p->transformations.setPosition(oldPos); // Natychmiastowe cofnięcie!
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


PreviewContext buildPreviewContext(const AppState& state,const TransformManager& tm, const GuiManager& gui, Vect3 cursorPosition, Vect3 centerOfSelection, bool isVirtualSelected)
{
    PreviewContext ctx;
    ctx.isTransforming = (state.inputMode == INPUT_MOUSE && tm.isTransformationActive) || (state.inputMode == INPUT_GUI);
    ctx.isLocal = (state.transformMode == LOCAL);

    ctx.isVirtualSelected = isVirtualSelected;

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