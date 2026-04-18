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

    for (size_t i = 0; i < deBoor.size() - 3; ++i)
    {
        Vect3 d0 = deBoor[i]; Vect3 d1 = deBoor[i+1]; Vect3 d2 = deBoor[i+2]; Vect3 d3 = deBoor[i+3];
        bernsteinPts.push_back((d0 + d1 * 4.0f + d2) * (1.0f / 6.0f));
        bernsteinPts.push_back((d1 * 2.0f + d2) * (1.0f / 3.0f));
        bernsteinPts.push_back((d1 + d2 * 2.0f) * (1.0f / 3.0f));
        bernsteinPts.push_back((d1 + d2 * 4.0f + d3) * (1.0f / 6.0f));
    }

    std::vector<Vect3> cleanPts;
    for (size_t i = 0; i < bernsteinPts.size(); ++i)
    {
        if (i % 4 != 3 || i == bernsteinPts.size() - 1)
            cleanPts.push_back(bernsteinPts[i]);
    }
    return cleanPts;
}

std::vector<Vect3> SceneBezierC2::getLiveBernsteinPoints(const PreviewContext& ctx)
{
    cleanExpiredPoints();

    int numD = points.size();
    if (numD < 4)
    {
        virtualPoints.clear();
        lastDeBoorPositions.clear();
        dragStartDeBoor.clear();
        wasTransforming = false;
        return {};
    }

    bool structureChanged = (lastDeBoorPositions.size() != numD);
    bool basisChanged = (currentBasis != lastBasis);
    bool positionsChanged = false;

    lastBasis = currentBasis;
    if (structureChanged)
    {
        lastDeBoorPositions.resize(numD, Vect3(0,0,0));
        dragStartDeBoor.resize(numD, Vect3(0,0,0));
    }

    // 1. ZBIERANIE FIZYCZNYCH POZYCJI ZE ŚWIATA
    std::vector<Vect3> pureD(numD, Vect3(0.0));
    bool deBoorSelected = false;

    for (int i = 0; i < numD; ++i)
    {
        if (auto p = points[i].lock())
        {
            pureD[i] = p->transformations.getPosition();
            if (p->isSelected)
                deBoorSelected = true;

            if (!ctx.isTransforming)
            {
                if ((pureD[i] - lastDeBoorPositions[i]).length() > 0.0001f)
                {
                    positionsChanged = true;
                    lastDeBoorPositions[i] = pureD[i];
                }
            }
        }
    }

    // =================================================================
    // MAGIA ZAMROŻONEJ BAZY (Zapobiega odlatywaniu punktów w kosmos!)
    // =================================================================
    if (ctx.isTransforming && !wasTransforming)
    {
        // Myszka wciśnięta w tej klatce: ZAMRAŻAMY OBECNY STAN ŚWIATA
        dragStartDeBoor = pureD;
        wasTransforming = true;
    }
    else if (!ctx.isTransforming && wasTransforming)
    {
        // Myszka puszczona: odblokowujemy system
        wasTransforming = false;
        lastDeBoorPositions = pureD;
    }

    // EARLY EXIT
    if (!ctx.isTransforming && !positionsChanged && !structureChanged && !basisChanged) {
        if (currentBasis == BezierBasisMode::B_SPLINE) {
            return calculateBernsteinPointsFrom(pureD);
        } else {
            std::vector<Vect3> currentP(virtualPoints.size());
            for (size_t i = 0; i < virtualPoints.size(); ++i) {
                currentP[i] = virtualPoints[i]->transformations.getPosition();
            }
            return currentP;
        }
    }

    // =================================================================
    // 2. WYLICZANIE POZYCJI
    // Jeśli jesteśmy w trakcie ruchu, UŻYWAMY TYLKO ZAMROŻONEJ BAZY!
    // =================================================================
    std::vector<Vect3> baseD = (ctx.isTransforming) ? dragStartDeBoor : pureD;

    // --- TRYB B-SPLINE ---
    if (currentBasis == BezierBasisMode::B_SPLINE) {
        virtualPoints.clear();
        std::vector<Vect3> liveD = baseD;

        if (ctx.isTransforming && deBoorSelected) {
            for (int i = 0; i < numD; ++i) {
                if (auto p = points[i].lock()) {
                    if (p->isSelected) {
                        Vect3 pos = baseD[i];
                        // Obliczamy deltę CZYSTO z zamrożonej bazy
                        if (ctx.isLocal) pos += ctx.localDeltaPos;
                        else
                        {
                            Vect4 p4(pos.x, pos.y, pos.z, 1.0f);
                            pos = (ctx.groupMat * p4).toVect3();
                        }
                        liveD[i] = pos;
                    }
                }
            }
        }
        return calculateBernsteinPointsFrom(liveD);
    }

    // --- TRYB BERNSTEIN ---
    int numP = (numD - 3) * 3 + 1;
    std::vector<Vect3> baseP = calculateBernsteinPointsFrom(baseD); // Zamrożona baza Bernsteina

    if (virtualPoints.size() != numP) {
        virtualPoints.clear();
        for (int i = 0; i < numP; ++i) {
            Transformations t; t.setPosition(baseP[i]);
            auto vp = std::make_shared<ScenePoint>("Wirtualny " + std::to_string(i), t);
            vp->Init(); vp->color[0] = 0; vp->color[1] = 1; vp->color[2] = 0;
            virtualPoints.push_back(vp);
        }
    }

    int movedP_Index = -1;
    Vect3 delta(0,0,0);
    for (int i = 0; i < numP; ++i) {
        if (virtualPoints[i]->isSelected) {
            movedP_Index = i;

            // OBLICZAMY DELTĘ CAŁKOWICIE NIEZALEŻNIE OD RZECZYWISTEJ POZYCJI KURSORA
            Vect3 startPos = baseP[i];
            Vect3 previewPos = startPos;
            if (ctx.isLocal) {
                previewPos.x += ctx.localDeltaPos.x;
                previewPos.y += ctx.localDeltaPos.y;
                previewPos.z += ctx.localDeltaPos.z;
            } else {
                Vect4 p4(startPos.x, startPos.y, startPos.z, 1.0f);
                Vect4 newP = ctx.groupMat * p4;
                previewPos = Vect3(newP.x, newP.y, newP.z);
            }
            delta = previewPos - startPos; // Czysty ruch kursora od momentu kliknięcia!
            break;
        }
    }

    std::vector<Vect3> liveD = baseD;
    bool bernsteinMaster = !deBoorSelected && movedP_Index != -1;

    if (ctx.isTransforming)
    {
        if (bernsteinMaster) {
            int d_index = 0;
            float multiplier = 1.5f;
            if (movedP_Index == 0) { d_index = 0; multiplier = 6.0f; }
            else if (movedP_Index == numP - 1) { d_index = numD - 1; multiplier = 6.0f; }
            else { d_index = (movedP_Index + 1) / 3 + 1; }

            if (d_index >= 0 && d_index < numD) {
                // Aplikujemy czystą deltę do zamrożonej bazy
                liveD[d_index] += delta * multiplier;

                // Aktualizujemy świat, by zapisać wypieczenie
                if (auto p = points[d_index].lock()) {
                    p->transformations.setPosition(liveD[d_index]);
                }
            }
        }
        else if (deBoorSelected)
        {
            for (int i = 0; i < numD; ++i) {
                if (auto p = points[i].lock()) {
                    if (p->isSelected) {
                        Vect3 pos = baseD[i];
                        if (ctx.isLocal) pos += ctx.localDeltaPos;
                        else { Vect4 p4(pos.x, pos.y, pos.z, 1.0f); pos = (ctx.groupMat * p4).toVect3(); }
                        liveD[i] = pos;
                    }
                }
            }
        }
    }

    // 3. WIZUALIZACJA
    std::vector<Vect3> liveP = calculateBernsteinPointsFrom(liveD);

    for (int i = 0; i < numP; ++i)
    {
        virtualPoints[i]->transformations.setPosition(liveP[i]);
    }

    return liveP;
}

void SceneBezierC2::DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode)
{
    std::vector<Vect3> bernsteinPts = getLiveBernsteinPoints(ctx);
    if (bernsteinPts.empty() || bernsteinPts.size() < 4) return;

    if (mode == GEOMETRY) RenderGeometryMode(bernsteinPts, shader, VP, winWidth, winHeight);
    else RenderLineStripMode(bernsteinPts, shader, VP, winWidth, winHeight);
}

void SceneBezierC2::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    std::vector<Vect3> bernsteinPts = getLiveBernsteinPoints(ctx);
    if (points.size() < 2 || !showPolygon) return;

    std::vector<Vect3> polyPoints;
    if (currentBasis == BezierBasisMode::B_SPLINE) {
        for(auto& wp : points) {
            if (auto p = wp.lock()) polyPoints.push_back(getPreviewPosition(p, ctx));
        }
    } else {
        polyPoints = bernsteinPts;
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