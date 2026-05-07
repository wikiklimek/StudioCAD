#include "sceneSplineInterpolating.h"
#include "previewFunctions.h"

SceneSplineInterpolating::SceneSplineInterpolating(std::string n, Transformations spawnTransform)
        : SceneBezier(std::move(n), spawnTransform, ObjectType::SplineInterpolating) {}



void SceneSplineInterpolating::UpdateVirtualPointsIfNeeded(const PreviewContext &ctx)
{
    cleanExpiredPoints();
    if (points.size() < 2)
    {
        virtualPoints.clear();
        segmentCoeffs.clear();
        return;
    }

    int n = points.size() - 1;

    // jest git bo czyszcze
    int expectedVpCount = (currentBasis == InterpolationBasisMode::BERNSTEIN) ? n * 3 : 0;
    int expectedCoeffsCount = (currentBasis == InterpolationBasisMode::ALGEBRAIC) ? n : 0;

    bool needsUpdate = ctx.anySelectionChanged || ctx.wasBaked ||
                       virtualPoints.size() != expectedVpCount ||
                       segmentCoeffs.size() != expectedCoeffsCount;

    std::vector<Vect3> liveP;
    for (auto& wp : points)
    {
        auto p = wp.lock();
        liveP.push_back(getPreviewPosition(p, ctx));

        if (p->wasGuiEdited ||
            (ctx.isTransforming && (p->isSelected || p->isSelectedAsDeBoore || p->selectedCurvesCount > 0)))
            needsUpdate = true;
    }

    if (!needsUpdate)
        return;

    std::vector<float> d(n);
    for (int i = 0; i < n; ++i)
        d[i] = std::max((liveP[i+1] - liveP[i]).length(), 0.001f);

    // Algorytm Thomasa
    std::vector<Vect3> c(n + 1, Vect3(0.0f));
    if (n > 1)
    {
        std::vector<float> alpha(n), beta(n), cp(n);
        std::vector<Vect3> R(n), Rp(n);

        for (int i = 1; i < n; ++i)
        {
            alpha[i] = d[i-1] / (d[i-1] + d[i]);
            beta[i] = d[i] / (d[i-1] + d[i]);
            R[i] = ((liveP[i+1]-liveP[i])*(1.0f/d[i]) - (liveP[i]-liveP[i-1])*(1.0f/d[i-1])) * (3.0f/(d[i-1]+d[i]));
        }

        cp[1] = beta[1] / 2.0f;
        Rp[1] = R[1] * 0.5f;

        for (int i = 2; i < n; ++i)
        {
            float den = 2.0f - alpha[i] * cp[i-1];
            cp[i] = beta[i] / den;
            Rp[i] = (R[i] - Rp[i-1] * alpha[i]) * (1.0f/den);
        }

        for (int i = n-1; i > 0; --i)
            c[i] = Rp[i] - c[i+1] * cp[i];
    }

    // czyszcze
    segmentCoeffs.clear();
    virtualPoints.clear();

    for (int i = 0; i < n; ++i)
    {
        Vect3 a_i = liveP[i];
        Vect3 c_i = c[i];
        Vect3 b_i = (liveP[i+1] - liveP[i]) * (1.0f/d[i]) - (c[i]*2.0f + c[i+1])*(d[i]/3.0f);

        if (currentBasis == InterpolationBasisMode::ALGEBRAIC)
        {
            Vect3 d_i_coeff = (c[i+1] - c[i]) * (1.0f / (3.0f * d[i]));
            segmentCoeffs.push_back({a_i, b_i, c_i, d_i_coeff, d[i]});
        }
        else // BERNSTEIN
        {
            Vect3 v1 = a_i + b_i * (d[i]/3.0f);
            Vect3 v2 = v1 + (b_i + c_i * d[i]) * (d[i]/3.0f);

            auto makeVP = [&](Vect3 pos){
                Transformations t; t.setPosition(pos);
                auto vp = std::make_shared<ScenePoint>("V", t);
                vp->Init();
                vp->isVirtual = true;
                vp->color[0]=0;
                vp->color[1]=1;
                vp->color[2]=1;
                return vp;
            };
            virtualPoints.push_back(makeVP(liveP[i]));
            virtualPoints.push_back(makeVP(v1));
            virtualPoints.push_back(makeVP(v2));
        }
    }
}



void SceneSplineInterpolating::DrawBezier(Shader& shader, Mat4 VP, int winWidth, int winHeight, const PreviewContext& ctx, BezierDrawMode mode)
{
    UpdateVirtualPointsIfNeeded(ctx);
    if (points.size() < 2)
        return; // Zabezpieczenie przed rysowaniem pustej krzywej

    shader.use();
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, color);

    if (currentBasis == InterpolationBasisMode::ALGEBRAIC)
    {
        for (auto& seg : segmentCoeffs)
        {
            shader.setVec3("a_coeff", seg.a.x, seg.a.y, seg.a.z);
            shader.setVec3("b_coeff", seg.b.x, seg.b.y, seg.b.z);
            shader.setVec3("c_coeff", seg.c.x, seg.c.y, seg.c.z);
            shader.setVec3("d_coeff", seg.d_coeff.x, seg.d_coeff.y, seg.d_coeff.z);
            shader.setFloat("d_len", seg.d_len);
            shader.setInt("segmentCount", 100);
            glDrawArrays(GL_POINTS, 0, 1);
        }
    }
    else
    {
        std::vector<Vect3> pts;
        for (auto& vp : virtualPoints)
        {
            pts.push_back(vp->transformations.getPosition());
        }

        //ostatni wezel zaginal z virtualnych punktów
        if (!points.empty())
        {
            if (auto p = points.back().lock())
            {
                pts.push_back(getPreviewPosition(p, ctx));
            }
        }

        if (mode == GEOMETRY)
            RenderGeometryMode(pts, shader, VP, winWidth, winHeight, BezierBasisMode::BERNSTEIN);
        else
            RenderLineStripMode(pts, shader, VP, winWidth, winHeight, BezierBasisMode::BERNSTEIN);
    }
}

void SceneSplineInterpolating::Draw(Shader& shader)
{
    if (currentBasis == InterpolationBasisMode::BERNSTEIN)
        for (auto& vp : virtualPoints)
            vp->Draw(shader);
}

void SceneSplineInterpolating::DrawPolygon(Shader& lineShader, const PreviewContext& ctx)
{
    if (points.size() < 2 || !showPolygon)
        return;

    std::vector<Vect3> polyPoints;

    if (currentBasis == InterpolationBasisMode::ALGEBRAIC)
    {
        for (auto& wp : points)
        {
            if (auto p = wp.lock())
                polyPoints.push_back(getPreviewPosition(p, ctx));
        }
    }
    else // BERNSTEIN
    {
        for (auto& vp : virtualPoints)
        {
            polyPoints.push_back(vp->transformations.getPosition());
        }

        // ostatni wirtualny zaginał
        if (!points.empty())
        {
            if (auto p = points.back().lock())
            {
                polyPoints.push_back(getPreviewPosition(p, ctx));
            }
        }
    }

    RenderPolygon(polyPoints, lineShader);
}


void SceneSplineInterpolating::Draw(Shader& shader, Mat4 parentMatrix)
{
    if (points.empty())
        return;

    if (currentBasis == InterpolationBasisMode::BERNSTEIN)
    {
        for (auto& vp : virtualPoints)
            vp->Draw(shader);
            //vp->Draw(shader, parentMatrix);
    }
}