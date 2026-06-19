#include "sceneSurface.h"

void SceneSurfaceC2::Init()
{
    InitBuffers();

    int patchesV = sizeV - 3;
    int patchesU = isCylinder ? (sizeU - 1) : (sizeU - 3);

    for (int pv = 0; pv < patchesV; ++pv)
    {
        for (int pu = 0; pu < patchesU; ++pu)
        {
            int startU = pu;
            int startV = pv;

            for (int j = 0; j < 4; ++j)
            {
                for (int i = 0; i < 4; ++i)
                {
                    int u_idx = startU + i;

                    // C2 CYLINDER
                    if (isCylinder)
                    {
                        if (u_idx >= sizeU)
                        {
                            u_idx = u_idx - sizeU + 1;
                        }
                    }

                    patchIndices.push_back((startV + j) * sizeU + u_idx);
                }
            }
        }
    }

    InitPolygonAndUpload();
}

Vect3 SceneSurfaceC2::EvaluatePos(float u, float v) const
{
    int patchesU = isCylinder ? (sizeU - 1) : (sizeU - 3);
    int patchesV = sizeV - 3;

    float u_scaled = u * patchesU;
    float v_scaled = v * patchesV;
    int pu = std::clamp((int)u_scaled, 0, patchesU - 1);
    int pv = std::clamp((int)v_scaled, 0, patchesV - 1);
    float local_u = u_scaled - pu;
    float local_v = v_scaled - pv;

    Vect3 pos(0.0f);
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            int u_idx = pu + i;
            if (isCylinder && u_idx >= sizeU) u_idx = u_idx - sizeU + 1;
            int v_idx = pv + j;

            if (auto pt = points[v_idx * sizeU + u_idx].lock()) {
                pos += pt->transformations.getPosition() * (BSpline(i, local_u) * BSpline(j, local_v));
            }
        }
    }
    return pos;
}

Vect3 SceneSurfaceC2::EvaluateDu(float u, float v) const
{
    int patchesU = isCylinder ? (sizeU - 1) : (sizeU - 3);
    int patchesV = sizeV - 3;

    float u_scaled = u * patchesU;
    float v_scaled = v * patchesV;
    int pu = std::clamp((int)u_scaled, 0, patchesU - 1);
    int pv = std::clamp((int)v_scaled, 0, patchesV - 1);
    float local_u = u_scaled - pu;
    float local_v = v_scaled - pv;

    Vect3 du(0.0f);
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            int u_idx = pu + i;
            if (isCylinder && u_idx >= sizeU) u_idx = u_idx - sizeU + 1;
            int v_idx = pv + j;

            if (auto pt = points[v_idx * sizeU + u_idx].lock()) {
                du += pt->transformations.getPosition() * (dBSpline(i, local_u) * BSpline(j, local_v));
            }
        }
    }
    return du * (float)patchesU;
}

Vect3 SceneSurfaceC2::EvaluateDv(float u, float v) const
{
    int patchesU = isCylinder ? (sizeU - 1) : (sizeU - 3);
    int patchesV = sizeV - 3;

    float u_scaled = u * patchesU;
    float v_scaled = v * patchesV;
    int pu = std::clamp((int)u_scaled, 0, patchesU - 1);
    int pv = std::clamp((int)v_scaled, 0, patchesV - 1);
    float local_u = u_scaled - pu;
    float local_v = v_scaled - pv;

    Vect3 dv(0.0f);
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            int u_idx = pu + i;
            if (isCylinder && u_idx >= sizeU) u_idx = u_idx - sizeU + 1;
            int v_idx = pv + j;

            if (auto pt = points[v_idx * sizeU + u_idx].lock()) {
                dv += pt->transformations.getPosition() * (BSpline(i, local_u) * dBSpline(j, local_v));
            }
        }
    }
    return dv * (float)patchesV;
}