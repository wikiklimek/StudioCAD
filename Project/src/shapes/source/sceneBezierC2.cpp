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
                float multiplier = 1.5f;

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
    int currentNumP = (int)virtualPoints.size();

    bool basisChangedToBernstein = (currentBasis != lastBasis) && (currentBasis == BezierBasisMode::BERNSTEIN);


    // ostatnie to dlatego ze na povczarku punktów nie ma
    bool needsFullRebuild = ctx.anySelectionChanged || ctx.wasBaked || basisChangedToBernstein ||
            (currentNumP > expectedNumP) || (currentNumP == 0 && currentBasis == BezierBasisMode::BERNSTEIN);

    bool haveToAddPoints = !needsFullRebuild && (currentNumP < expectedNumP);

    if(!ctx.isTransforming && !needsFullRebuild && !haveToAddPoints)
    {
        return;
    }

    // jakie punkty deBoora zmaniły pozycje
    std::vector<int> dirtyIndices;
    std::vector<Vect3> liveD(numD, Vect3(0.0f));

    for (int i = 0; i < numD; ++i)
    {
        auto p = points[i].lock();

        liveD[i] = getPreviewPosition(p, ctx);

        if (p->wasGuiEdited ||
            (ctx.isTransforming && (p->isSelected || p->isSelectedAsDeBoore || p->selectedCurvesCount > 0 || p->isSelectedViaPatch)))
        {
            dirtyIndices.push_back(i);
        }
    }

    if (dirtyIndices.empty() && !needsFullRebuild && !haveToAddPoints)
    {
        return;
    }


    if (needsFullRebuild)
    {
        rebuildAllVirtualPoints(liveD);
        return;
    }


    if (haveToAddPoints)
    {
        // Dodano nowe punkty De Boora
        int oldNumD = (currentNumP - 1) / 3 + 3;
        addVirtualPoints(oldNumD, numD, liveD);
    }
    if (!dirtyIndices.empty())
    {
        updateAffectedVirtualPoints(dirtyIndices, liveD);
    }
}



void SceneBezierC2::rebuildAllVirtualPoints(const std::vector<Vect3>& liveD)
{
    virtualPoints.clear();
    std::vector<Vect3> initP = calculateBernsteinPointsFrom(liveD);

    for (int i = 0; i < initP.size(); ++i)
    {
        Transformations t;
        t.setPosition(initP[i]);
        auto vp = std::make_shared<ScenePoint>("Wirtualny " + std::to_string(i), t);
        vp->Init();
        vp->color[0] = 0;
        vp->color[1] = 1;
        vp->color[2] = 0;
        vp->isVirtual = true;
        virtualPoints.push_back(vp);
    }
}

void SceneBezierC2::addVirtualPoints(int oldNumD, int newNumD, const std::vector<Vect3>& liveD)
{
    // nowe punkty berensteina dodane na koniec, stare pozostaja takie same
    // Pętla leci tylko od miejsca w którym skończyliśmy ostatnio
    for (int j = oldNumD - 3; j <= newNumD - 4; ++j)
    {
        Vect3 d1 = liveD[j+1];
        Vect3 d2 = liveD[j+2];
        Vect3 d3 = liveD[j+3];

        // 3 nowe punkty (pierwszy to ostatni stary węzeł, czyli pomijamy)
        Vect3 p1 = (d1 * 2.0f + d2) * (1.0f / 3.0f);
        Vect3 p2 = (d1 + d2 * 2.0f) * (1.0f / 3.0f);
        Vect3 p3 = (d1 + d2 * 4.0f + d3) * (1.0f / 6.0f);

        auto makeVP = [](Vect3 pos, int idx) {
            Transformations t; t.setPosition(pos);
            auto vp = std::make_shared<ScenePoint>("Wirtualny " + std::to_string(idx), t);
            vp->Init();
            vp->color[0] = 0;
            vp->color[1] = 1;
            vp->color[2] = 0;
            vp->isVirtual = true;
            return vp;
        };

        int b = virtualPoints.size();
        virtualPoints.push_back(makeVP(p1, b));
        virtualPoints.push_back(makeVP(p2, b+1));
        virtualPoints.push_back(makeVP(p3, b+2));
    }
}

void SceneBezierC2::updateAffectedVirtualPoints(const std::vector<int>& dirtyDeBoorIndices, const std::vector<Vect3>& liveD)
{
    int numSegments = liveD.size() - 3;
    std::vector<bool> dirtySegments(numSegments, false);


    for (int idx : dirtyDeBoorIndices)
    {
        int startSeg = std::max(0, idx - 3);
        int endSeg = std::min(numSegments - 1, idx);
        for (int j = startSeg; j <= endSeg; ++j)
            dirtySegments[j] = true;
    }


    for (int j = 0; j < numSegments; ++j)
    {
        if (dirtySegments[j])
        {
            Vect3 d0 = liveD[j];
            Vect3 d1 = liveD[j+1];
            Vect3 d2 = liveD[j+2];
            Vect3 d3 = liveD[j+3];

            virtualPoints[3*j]->transformations.setPosition((d0 + d1 * 4.0f + d2) * (1.0f / 6.0f));
            virtualPoints[3*j + 1]->transformations.setPosition((d1 * 2.0f + d2) * (1.0f / 3.0f));
            virtualPoints[3*j + 2]->transformations.setPosition((d1 + d2 * 2.0f) * (1.0f / 3.0f));
            virtualPoints[3*j + 3]->transformations.setPosition((d1 + d2 * 4.0f + d3) * (1.0f / 6.0f));
        }
    }
}


void SceneBezierC2::DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode)
{
    if(points.empty())
    {
        pendingDelete = true;
        return;
    }

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

    //MUSI BYC NA KONCY WYKONYWANE
    lastBasis = currentBasis;
}

void SceneBezierC2::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    if(points.empty())
    {
        return;
    }

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

void SceneBezierC2::Draw(Shader& shader)
{

    if(points.empty())
    {
        return;
    }

    if (currentBasis == BezierBasisMode::BERNSTEIN)
        for (auto& vp : virtualPoints)
            vp->Draw(shader);
}

void SceneBezierC2::Draw(Shader& shader, Mat4 parentMatrix)
{
    if(points.empty())
    {
        return;
    }

    if (currentBasis == BezierBasisMode::BERNSTEIN)
        for (auto& vp : virtualPoints)
            vp->Draw(shader);
            //vp->Draw(shader, parentMatrix);
}