#include "gregoryGrid.h"
#include "sceneGregoryPatch.h"
#include <cmath>



// Pomocniczy algorytm de Casteljau do dzielenia krzywej w t=0.5
void SubdivideBezier(const Vect3 P[4], Vect3 L[4], Vect3 R[4])
{
    Vect3 p01 = (P[0] + P[1]) * 0.5f;
    Vect3 p12 = (P[1] + P[2]) * 0.5f;
    Vect3 p23 = (P[2] + P[3]) * 0.5f;
    Vect3 p012 = (p01 + p12) * 0.5f;
    Vect3 p123 = (p12 + p23) * 0.5f;
    Vect3 p0123 = (p012 + p123) * 0.5f;

    L[0] = P[0]; L[1] = p01; L[2] = p012; L[3] = p0123;
    R[0] = p0123; R[1] = p123; R[2] = p23; R[3] = P[3];
}


std::shared_ptr<SceneGregoryPatch> GenerateGregoryPatchForHole(
        const HoleCycle& hole, std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    // Aplikacja obsługuje otwory składające się z 3 krawędzi
    if (hole.edges.size() != 3) return nullptr;

    auto patch = std::make_shared<SceneGregoryPatch>("Plat Gregorego", Transformations());

    // =======================================================
    // NOWE: Po prostu przepisujemy gotowe ID z dziury!
    // =======================================================
    patch->id_gregory = hole.id_gregory;

    patch->points.resize(60);

    // KROK 1 & 2 & 4: Zbieranie danych z Beziera i de Casteljau
    Vect3 E[3][4], IE[3][4];
    Vect3 L[3][4], R[3][4], IL[3][4], IR[3][4];

    for (int i = 0; i < 3; ++i)
    {
        for(int j=0; j<4; ++j)
            E[i][j] = hole.edges[i].p[j]->transformations.getPosition();

        GetInnerRow(hole.edges[i], IE[i]);

        SubdivideBezier(E[i], L[i], R[i]);
        SubdivideBezier(IE[i], IL[i], IR[i]);
    }

    // =======================================================
    // KROK 1 & 3: Punkt Centralny Pc i Szkielet Wewnętrzny
    // (Zgodnie z algorytmem: "Fill in of a triangular gap...")
    // =======================================================

    Vect3 Q[3]; // Punkty Qi z wykładu
    for (int i = 0; i < 3; ++i)
    {
        // Wymuszenie rygorystycznego C1 dla startu krzywej promienistej (Tworzy punkt Qi)
        Vect3 midCrossDeriv = L[i][3] - IL[i][3];
        Q[i] = L[i][3] + midCrossDeriv;
    }

    // Punkt Centralny Pc to po prostu środek ciężkości punktów Q
    // Gwarantuje to, że Pc leży idealnie w płaszczyźnie rozpiętej przez wektory C1!
    Vect3 Pc_pos = (Q[0] + Q[1] + Q[2]) / 3.0f;

    auto pt_Pc = std::make_shared<ScenePoint>("Gregory Center (Pc)", Pc_pos);
    pt_Pc->Init();
    pt_Pc->color[0]=1;
    pt_Pc->color[1]=0;
    pt_Pc->color[2]=1;
    sceneObjects.push_back(pt_Pc);

    Vect3 Rad[3][4];
    std::shared_ptr<ScenePoint> pt_Rad[3][4];

    for (int i = 0; i < 3; ++i)
    {
        Rad[i][0] = L[i][3]; // Midpoint
        Rad[i][1] = Q[i];    // Punkt Qi (C1)

        // Zgodnie z wykładem: P1i = 0.5 * (P + Qi). Tworzy to idealnie gładką kopułę bez rzutowania!
        Rad[i][2] = Pc_pos + (Q[i] - Pc_pos) * 0.5f;

        Rad[i][3] = Pc_pos;  // Punkt centralny

        // Inicjalizacja fizycznych obiektów na scenie
        pt_Rad[i][0] = std::make_shared<ScenePoint>("Midpoint_M"+std::to_string(i), Rad[i][0]); pt_Rad[i][0]->Init(); sceneObjects.push_back(pt_Rad[i][0]);
        pt_Rad[i][1] = std::make_shared<ScenePoint>("Rad_"+std::to_string(i)+"_1", Rad[i][1]); pt_Rad[i][1]->Init(); sceneObjects.push_back(pt_Rad[i][1]);
        pt_Rad[i][2] = std::make_shared<ScenePoint>("Rad_"+std::to_string(i)+"_2", Rad[i][2]); pt_Rad[i][2]->Init(); sceneObjects.push_back(pt_Rad[i][2]);
        pt_Rad[i][3] = pt_Pc;
    }

    std::shared_ptr<ScenePoint> pt_L[3][4], pt_R[3][4];
    for (int i = 0; i < 3; ++i)
    {
        pt_L[i][0] = hole.edges[i].p[0];
        pt_L[i][1] = std::make_shared<ScenePoint>("L_"+std::to_string(i)+"_1", L[i][1]); pt_L[i][1]->Init(); sceneObjects.push_back(pt_L[i][1]);
        pt_L[i][2] = std::make_shared<ScenePoint>("L_"+std::to_string(i)+"_2", L[i][2]); pt_L[i][2]->Init(); sceneObjects.push_back(pt_L[i][2]);
        pt_L[i][3] = pt_Rad[i][0];

        pt_R[i][0] = pt_Rad[i][0];
        pt_R[i][1] = std::make_shared<ScenePoint>("R_"+std::to_string(i)+"_1", R[i][1]); pt_R[i][1]->Init(); sceneObjects.push_back(pt_R[i][1]);
        pt_R[i][2] = std::make_shared<ScenePoint>("R_"+std::to_string(i)+"_2", R[i][2]); pt_R[i][2]->Init(); sceneObjects.push_back(pt_R[i][2]);
        pt_R[i][3] = hole.edges[i].p[3];
    }

    // Składanie 3 sub-płatów do jednego obiektu
    for (int i = 0; i < 3; ++i)
    {
        int prev = (i + 2) % 3;
        int curr = i;
        int offset = i * 20;

        std::shared_ptr<ScenePoint> bnd_V0[4] = { pt_L[curr][0], pt_L[curr][1], pt_L[curr][2], pt_L[curr][3] };
        std::shared_ptr<ScenePoint> bnd_U0[4] = { pt_R[prev][3], pt_R[prev][2], pt_R[prev][1], pt_R[prev][0] };
        std::shared_ptr<ScenePoint> bnd_V1[4] = { pt_Rad[prev][0], pt_Rad[prev][1], pt_Rad[prev][2], pt_Rad[prev][3] };
        std::shared_ptr<ScenePoint> bnd_U1[4] = { pt_Rad[curr][0], pt_Rad[curr][1], pt_Rad[curr][2], pt_Rad[curr][3] };

        // =======================================================
        // KROK 4: Wymuszenie ścisłego C1 (Zgodność Parametryczna)
        // =======================================================

        // Obliczamy surowe wektory pochodnych cząstkowych z płatów Beziera
        Vect3 crossDeriv_prev_1 = R[prev][2] - IR[prev][2];
        Vect3 crossDeriv_prev_2 = R[prev][1] - IR[prev][1];

        // Aplikujemy pochodne do płata Gregory'ego od strony krzywej U=0
        Vect3 p11u_pos = R[prev][2] + crossDeriv_prev_1;
        Vect3 p21u_pos = R[prev][1] + crossDeriv_prev_2;

        Vect3 crossDeriv_curr_1 = L[curr][1] - IL[curr][1];
        Vect3 crossDeriv_curr_2 = L[curr][2] - IL[curr][2];

        // Aplikujemy pochodne do płata Gregory'ego od strony krzywej V=0
        Vect3 p11v_pos = L[curr][1] + crossDeriv_curr_1;
        Vect3 p12v_pos = L[curr][2] + crossDeriv_curr_2;

        // --- KROK 5: Wewnętrzny Rząd (Zgodność G1 przy promieniach) ---
        Vect3 P22_pos = Pc_pos + (Rad[prev][2] - Pc_pos)*0.5f + (Rad[curr][2] - Pc_pos)*0.5f;

        Vect3 p12u_pos = Rad[curr][2] + (Rad[curr][2] - P22_pos);
        Vect3 p21v_pos = Rad[prev][2] + (Rad[prev][2] - P22_pos);

        auto p11u = std::make_shared<ScenePoint>("p11u_"+std::to_string(i), p11u_pos); p11u->Init(); sceneObjects.push_back(p11u);
        auto p21u = std::make_shared<ScenePoint>("p21u_"+std::to_string(i), p21u_pos); p21u->Init(); sceneObjects.push_back(p21u);
        auto p11v = std::make_shared<ScenePoint>("p11v_"+std::to_string(i), p11v_pos); p11v->Init(); sceneObjects.push_back(p11v);
        auto p12v = std::make_shared<ScenePoint>("p12v_"+std::to_string(i), p12v_pos); p12v->Init(); sceneObjects.push_back(p12v);
        auto p12u = std::make_shared<ScenePoint>("p12u_"+std::to_string(i), p12u_pos); p12u->Init(); sceneObjects.push_back(p12u);
        auto p21v = std::make_shared<ScenePoint>("p21v_"+std::to_string(i), p21v_pos); p21v->Init(); sceneObjects.push_back(p21v);
        auto p22u = std::make_shared<ScenePoint>("p22u_"+std::to_string(i), P22_pos);  p22u->Init(); sceneObjects.push_back(p22u);

        // --- ZAPIS DO STRUKTURY 20-PUNKTOWEJ ---
        patch->points[offset + 0] = bnd_V0[0];
        patch->points[offset + 1] = bnd_V0[1];
        patch->points[offset + 2] = bnd_V0[2];
        patch->points[offset + 3] = bnd_V0[3];

        patch->points[offset + 4] = bnd_U0[1];
        patch->points[offset + 5] = p11u;
        patch->points[offset + 6] = p11v;
        patch->points[offset + 7] = p12u;
        patch->points[offset + 8] = p12v;
        patch->points[offset + 9] = bnd_U1[1];

        patch->points[offset + 10]= bnd_U0[2];
        patch->points[offset + 11]= p21u;
        patch->points[offset + 12]= p21v;
        patch->points[offset + 13]= p22u;
        patch->points[offset + 14]= p22u;
        patch->points[offset + 15]= bnd_U1[2];

        patch->points[offset + 16]= bnd_V1[0];
        patch->points[offset + 17]= bnd_V1[1];
        patch->points[offset + 18]= bnd_V1[2];
        patch->points[offset + 19]= bnd_V1[3];
    }

    for(auto& wp : patch->points)
        if(auto p = wp.lock())
            p->globalSurfacesCount++;

    patch->Init();

    return patch;
}