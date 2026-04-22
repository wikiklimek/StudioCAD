#include "sceneBezierC2.h"
#include "previewFunctions.h"
#include "screenInteractions.h"
#include <algorithm>

SceneBezierC2::SceneBezierC2(std::string n, Transformations spawnTransform)
        : SceneBezier(std::move(n), spawnTransform, ObjectType::BezierCurveC2) {}

std::vector<Vect3> SceneBezierC2::calculateBernsteinPointsFrom(const std::vector<Vect3>& deBoor)
{
    std::vector<Vect3> bernsteinPts;
    if (deBoor.size() < 4) return bernsteinPts;

    for (size_t i = 0; i < deBoor.size() - 3; ++i) {
        Vect3 d0 = deBoor[i]; Vect3 d1 = deBoor[i+1]; Vect3 d2 = deBoor[i+2]; Vect3 d3 = deBoor[i+3];
        bernsteinPts.push_back((d0 + d1 * 4.0f + d2) * (1.0f / 6.0f));
        bernsteinPts.push_back((d1 * 2.0f + d2) * (1.0f / 3.0f));
        bernsteinPts.push_back((d1 + d2 * 2.0f) * (1.0f / 3.0f));
        bernsteinPts.push_back((d1 + d2 * 4.0f + d3) * (1.0f / 6.0f));
    }

    std::vector<Vect3> cleanPts;
    for (size_t i = 0; i < bernsteinPts.size(); ++i) {
        if (i % 4 != 3 || i == bernsteinPts.size() - 1) cleanPts.push_back(bernsteinPts[i]);
    }
    return cleanPts;
}

// =========================================================================
// FUNKCJA Z TWOJEJ INSTRUKCJI: OZNACZA PUNKTY DE BOORA
// =========================================================================
void SceneBezierC2::markAffectedDeBoorPoints()
{
    if (currentBasis != BezierBasisMode::BERNSTEIN)
        return;

    int expectedNumP = ((int)points.size() - 3) * 3 + 1;

    if (virtualPoints.size() != expectedNumP)
        return;

    for (int i = 0; i < virtualPoints.size(); ++i)
    {
        if (virtualPoints[i]->isSelected)
        {
            int numD = (int)points.size();

            if (i % 3 == 0)
            {
                // KRAŃCOWE I WĘZŁY - Teraz wszystkie używają tego samego, ogólnego wzoru!
                int d_index = (i + 1) / 3 + 1;
                float multiplier = 1.5f; // Zawsze 1.5f, zniknęło mnożenie x6

                if (d_index >= 0 && d_index < numD)
                {
                    if (auto p = points[d_index].lock())
                    {
                        p->isSelectedAsDeBoore = true;
                        p->virtualWeight = multiplier;
                    }
                }
            }
            else
            {
                // ŚRODKI SEGMENTÓW - Dwa punkty De Boora (Bez zmian)
                int d1 = (i + 2) / 3;
                int d2 = d1 + 1;
                if (d1 >= 0 && d1 < numD)
                {
                    if (auto p = points[d1].lock())
                    {
                        p->isSelectedAsDeBoore = true;
                        p->virtualWeight = 1.0f;
                    }
                }

                if (d2 >= 0 && d2 < numD)
                {
                    if (auto p = points[d2].lock())
                    {
                        p->isSelectedAsDeBoore = true;
                        p->virtualWeight = 1.0f;
                    }
                }
            }
            break; // Znaleźliśmy ten jeden zaznaczony punkt, koniec szukania
        }
    }
}

// =========================================================================
// ULTRA CZYSTA, BEZSTANOWA AKTUALIZACJA
// =========================================================================
void SceneBezierC2::UpdateVirtualPointsIfNeeded(const PreviewContext& ctx)
{
    cleanExpiredPoints();
    int numD = (int)points.size();
    if (numD < 4)
    {
        virtualPoints.clear();
        return;
    }

    int expectedNumP = (numD - 3) * 3 + 1;
    bool numberOfPointsChanged = (virtualPoints.size() != expectedNumP);
    bool haveToAddPoints = virtualPoints.size() < expectedNumP;
    bool basisChangedToBernstein = (currentBasis != lastBasis) && currentBasis == BezierBasisMode::BERNSTEIN;
    lastBasis = currentBasis;


    // Szukamy edycji z GUI
    bool guiEdited = false;
    for (int i = 0; i < numD; ++i)
    {
        if (auto p = points[i].lock())
        {
            if (p->wasGuiEdited)
                guiEdited = true;
        }
    }

    // --- MEGA OPTYMALIZACJA ---
    // Zero ruchu z myszki, zero ruchu w GUI, struktura nienaruszona? UCIEKAMY!
    if (!ctx.isTransforming && !guiEdited && !numberOfPointsChanged && !basisChangedToBernstein)
    {
        return;
    }

    // Inicjalizacja/Odbudowa jeśli zmieniono strukturę lub bazę
    if (numberOfPointsChanged || basisChangedToBernstein)
    {
        virtualPoints.clear();

        //budujemy na nowo punkty z pozycja zerowa, potem bedziemy liczyc z preview pozycje
        for (int i = 0; i < expectedNumP; ++i)
        {
            Transformations t;
            auto vp = std::make_shared<ScenePoint>("Wirtualny " + std::to_string(i), t);
            vp->Init();
            vp->color[0] = 0;
            vp->color[1] = 1;
            vp->color[2] = 0;
            vp->isVirtual = true;
            virtualPoints.push_back(vp);
        }

    }

    // Bez żadnych sztuczek! Program (getPreviewPosition) sam pociągnął De Boory
    // z uwzględnieniem naszych flag. My tylko je odczytujemy.
    std::vector<Vect3> liveD(numD, Vect3(0.0f));
    for (int i = 0; i < numD; ++i)
    {
        if (auto p = points[i].lock())
            liveD[i] = getPreviewPosition(p, ctx);
    }

    std::vector<Vect3> liveP = calculateBernsteinPointsFrom(liveD);
    for (int i = 0; i < expectedNumP; ++i)
    {
        virtualPoints[i]->transformations.setPosition(liveP[i]);
    }
}

// =========================================================================
// FUNKCJE RYSUJĄCE
// =========================================================================
void SceneBezierC2::DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode)
{
    std::vector<Vect3> Pts;

    if (currentBasis == BezierBasisMode::B_SPLINE)
    {
        cleanExpiredPoints();
        if (points.size() < 4)
            return;


        for (auto& wp : points)
        {
            if (auto p = wp.lock())
                Pts.push_back(getPreviewPosition(p, ctx));
        }


    }
    else // BezierBasisMode::BERNSTEIN
    {
        //tylko w tym miejscu to robie
        UpdateVirtualPointsIfNeeded(ctx);
        if (virtualPoints.size() < 2)
            return;


        for (auto& vp : virtualPoints)
            Pts.push_back(vp->transformations.getPosition());

    }

    if (mode == GEOMETRY)
        RenderGeometryMode(Pts, shader, VP, winWidth, winHeight, currentBasis);
    else
        RenderLineStripMode(Pts, shader, VP, winWidth, winHeight, currentBasis);
}

void SceneBezierC2::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    if (points.size() < 2 || !showPolygon)
        return;

    std::vector<Vect3> polyPoints;

    if (currentBasis == BezierBasisMode::B_SPLINE)
    {
        for(auto& wp : points)
        {
            if (auto p = wp.lock())
                polyPoints.push_back(getPreviewPosition(p, ctx));
        }
    }
    else
    {
        for(auto& vp : virtualPoints)
        {
            polyPoints.push_back(vp->transformations.getPosition());
        }
    }

    RenderPolygon(polyPoints, lineShader);
}

void SceneBezierC2::Draw(Shader& shader) {
    if (currentBasis == BezierBasisMode::BERNSTEIN)
        for (auto& vp : virtualPoints) vp->Draw(shader);
}

void SceneBezierC2::Draw(Shader& shader, Mat4 parentMatrix) {
    if (currentBasis == BezierBasisMode::BERNSTEIN)
        for (auto& vp : virtualPoints) vp->Draw(shader, parentMatrix);
}