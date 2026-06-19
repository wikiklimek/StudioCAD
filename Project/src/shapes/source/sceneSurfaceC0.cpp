#include "sceneSurface.h"

void SceneSurfaceC0::Init()
{
    InitBuffers();

    int patchesU = (sizeU - 1) / 3;
    int patchesV = (sizeV - 1) / 3;

    for (int pv = 0; pv < patchesV; ++pv)
    {
        for (int pu = 0; pu < patchesU; ++pu)
        {
            int startU = pu * 3;
            int startV = pv * 3;

            for (int j = 0; j < 4; ++j)
            {
                for (int i = 0; i < 4; ++i)
                {
                    patchIndices.push_back((startV + j) * sizeU + (startU + i));
                }
            }
        }
    }

    InitPolygonAndUpload();
}


Vect3 SceneSurfaceC0::EvaluatePos(float u, float v) const
{
    int patchesU = (sizeU - 1) / 3;
    int patchesV = (sizeV - 1) / 3;

    // Przeliczenie z globalnego [0,1] na lokalne indeksy i [0,1]
    float u_scaled = u * patchesU;
    float v_scaled = v * patchesV;
    int pu = std::clamp((int)u_scaled, 0, patchesU - 1);
    int pv = std::clamp((int)v_scaled, 0, patchesV - 1);
    float local_u = u_scaled - pu;
    float local_v = v_scaled - pv;

    Vect3 pos(0.0f);
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            int u_idx = pu * 3 + i;
            int v_idx = pv * 3 + j;
            if (auto pt = points[v_idx * sizeU + u_idx].lock()) {
                pos += pt->transformations.getPosition() * (Bernstein(i, local_u) * Bernstein(j, local_v));
            }
        }
    }
    return pos;
}

Vect3 SceneSurfaceC0::EvaluateDu(float u, float v) const
{
    int patchesU = (sizeU - 1) / 3;
    int patchesV = (sizeV - 1) / 3;

    float u_scaled = u * patchesU;
    float v_scaled = v * patchesV;
    int pu = std::clamp((int)u_scaled, 0, patchesU - 1);
    int pv = std::clamp((int)v_scaled, 0, patchesV - 1);
    float local_u = u_scaled - pu;
    float local_v = v_scaled - pv;

    Vect3 du(0.0f);
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            int u_idx = pu * 3 + i;
            int v_idx = pv * 3 + j;
            if (auto pt = points[v_idx * sizeU + u_idx].lock()) {
                du += pt->transformations.getPosition() * (dBernstein(i, local_u) * Bernstein(j, local_v));
            }
        }
    }
    // Reguła łańcuchowa (Chain rule): mnożymy wynik przez ilość płatów
    return du * (float)patchesU;
}

Vect3 SceneSurfaceC0::EvaluateDv(float u, float v) const
{
    int patchesU = (sizeU - 1) / 3;
    int patchesV = (sizeV - 1) / 3;

    float u_scaled = u * patchesU;
    float v_scaled = v * patchesV;
    int pu = std::clamp((int)u_scaled, 0, patchesU - 1);
    int pv = std::clamp((int)v_scaled, 0, patchesV - 1);
    float local_u = u_scaled - pu;
    float local_v = v_scaled - pv;

    Vect3 dv(0.0f);
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            int u_idx = pu * 3 + i;
            int v_idx = pv * 3 + j;
            if (auto pt = points[v_idx * sizeU + u_idx].lock()) {
                dv += pt->transformations.getPosition() * (Bernstein(i, local_u) * dBernstein(j, local_v));
            }
        }
    }
    return dv * (float)patchesV;
}