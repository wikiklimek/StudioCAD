#include "gregoryGrid.h"
#include "sceneGregoryPatch.h"
#include "previewFunctions.h"
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

    L[0] = P[0];
    L[1] = p01;
    L[2] = p012;
    L[3] = p0123;

    R[0] = p0123;
    R[1] = p123;
    R[2] = p23;
    R[3] = P[3];
}


std::shared_ptr<SceneGregoryPatch> GenerateGregoryPatchForHole(
        const HoleCycle& hole
        //,std::vector<std::shared_ptr<SceneObject>>& sceneObjects
        )
{
    // Aplikacja obsługuje otwory składające się z 3 krawędzi
    if (hole.edges.size() != 3)
        return nullptr;

    auto patch = std::make_shared<SceneGregoryPatch>("Plat Gregorego", Transformations());

    // Po prostu przepisujemy gotowe ID z dziury - bo jest unikalne
    patch->id_gregory = hole.id_gregory;

    // ZAKŁADAMY ZE BEDA TYLKO 3 KRAWEDZIE
    patch->points.resize(60);
    patch->bezierPatchPoints.resize(12);
    patch->bezierPatchPointsInner.resize(12);

    // Punkty z Beziera i de Casteljau w t=0.5
    Vect3 E[3][4], IE[3][4];
    Vect3 L[3][4], R[3][4], IL[3][4], IR[3][4];

    for (int i = 0; i < 3; ++i)
    {
        for(int j=0; j<4; ++j)
        {
            E[i][j] = hole.edges[i].p[j]->transformations.getPosition();
            patch->bezierPatchPoints[i * 4 + j] = hole.edges[i].p[j];
        }

        GetInnerRow(hole.edges[i], IE[i], &(patch->bezierPatchPointsInner[4 * i]));

        SubdivideBezier(E[i], L[i], R[i]);
        SubdivideBezier(IE[i], IL[i], IR[i]);
    }


    // Wyliczanie szkieletu
    Vect3 Rad[3][4];
    std::shared_ptr<ScenePoint> pt_Rad[3][4];
    Vect3 Q[3]; // Punkty pomocnicze Qi

    for (int i = 0; i < 3; ++i)
    {
        Vect3 E_pt = L[i][3];  // P_3i z obrazka
        Vect3 I_pt = IL[i][3];

        Rad[i][0] = E_pt; // Midpoint z beziera t = 0.5

        // P_2i z obrazka (warunek C1)
        Rad[i][1] = E_pt + (E_pt - I_pt);

        // Qi = P_3i + 3 * (P_2i - P_3i) / 2
        Q[i] = Rad[i][0] + (Rad[i][1] - Rad[i][0]) * 1.5f;
    }

    // Punkt Centralny P (Pc) <=> środek ciężkości punktów Q_i
    Vect3 Pc_pos = (Q[0] + Q[1] + Q[2]) / 3.0f;

    for (int i = 0; i < 3; ++i)
    {
        // Wyliczamy P_1i (nasze Rad[2]) ze wzoru na obrazku: P_1i = P + 2/3 * (Q_i - P)
        Rad[i][2] = Pc_pos + (Q[i] - Pc_pos) * (2.0f / 3.0f);
        Rad[i][3] = Pc_pos;
    }

    // Tworzenie obiektów punktów szkieletu
    auto pt_Pc = std::make_shared<ScenePoint>("Gregory Center (Pc)", Pc_pos);
    pt_Pc->Init();
    pt_Pc->color[0]=1; pt_Pc->color[1]=0; pt_Pc->color[2]=1;
    //sceneObjects.push_back(pt_Pc);

    for (int i = 0; i < 3; ++i)
    {
        pt_Rad[i][0] = std::make_shared<ScenePoint>("Midpoint_M"+std::to_string(i), Rad[i][0]);
        pt_Rad[i][0]->Init();
        //sceneObjects.push_back(pt_Rad[i][0]);

        pt_Rad[i][1] = std::make_shared<ScenePoint>("Rad_"+std::to_string(i)+"_1", Rad[i][1]);
        pt_Rad[i][1]->Init();
        //sceneObjects.push_back(pt_Rad[i][1]);

        pt_Rad[i][2] = std::make_shared<ScenePoint>("Rad_"+std::to_string(i)+"_2", Rad[i][2]);
        pt_Rad[i][2]->Init();
        //sceneObjects.push_back(pt_Rad[i][2]);

        pt_Rad[i][3] = pt_Pc;
    }


    //Krawędzie zewnętrzne (z Bezierem)
    std::shared_ptr<ScenePoint> pt_L[3][4], pt_R[3][4];
    for (int i = 0; i < 3; ++i)
    {
        // lewa strona - krawedz z bezierem
        //pt_L[i][0] = hole.edges[i].p[0];
        pt_L[i][0] = std::make_shared<ScenePoint>("L_"+std::to_string(i)+"_0", L[i][0]);
        pt_L[i][0]->Init();

        pt_L[i][1] = std::make_shared<ScenePoint>("L_"+std::to_string(i)+"_1", L[i][1]);
        pt_L[i][1]->Init();
        //sceneObjects.push_back(pt_L[i][1]);

        pt_L[i][2] = std::make_shared<ScenePoint>("L_"+std::to_string(i)+"_2", L[i][2]);
        pt_L[i][2]->Init();
        //sceneObjects.push_back(pt_L[i][2]);

        pt_L[i][3] = pt_Rad[i][0];

        // prawa strona - krawedz z bezierem
        pt_R[i][0] = pt_Rad[i][0];

        pt_R[i][1] = std::make_shared<ScenePoint>("R_"+std::to_string(i)+"_1", R[i][1]);
        pt_R[i][1]->Init();
        //sceneObjects.push_back(pt_R[i][1]);

        pt_R[i][2] = std::make_shared<ScenePoint>("R_"+std::to_string(i)+"_2", R[i][2]);
        pt_R[i][2]->Init();
        //sceneObjects.push_back(pt_R[i][2]);

        //pt_R[i][3] = hole.edges[i].p[3];
        pt_R[i][3] = std::make_shared<ScenePoint>("R_"+std::to_string(i)+"_3", R[i][3]);
        pt_R[i][3]->Init();
    }


    //Składanie 3 sub-płatów i punkty wewnętrzne
    for (int i = 0; i < 3; ++i)
    {
        int prev = (i + 2) % 3;
        int curr = i;
        int offset = i * 20;

        std::shared_ptr<ScenePoint> bnd_V0[4] = { pt_L[curr][0], pt_L[curr][1], pt_L[curr][2], pt_L[curr][3] };
        std::shared_ptr<ScenePoint> bnd_U0[4] = { pt_R[prev][3], pt_R[prev][2], pt_R[prev][1], pt_R[prev][0] };
        std::shared_ptr<ScenePoint> bnd_V1[4] = { pt_Rad[prev][0], pt_Rad[prev][1], pt_Rad[prev][2], pt_Rad[prev][3] };
        std::shared_ptr<ScenePoint> bnd_U1[4] = { pt_Rad[curr][0], pt_Rad[curr][1], pt_Rad[curr][2], pt_Rad[curr][3] };

        // Zewnętrzne krawędzie (Ścisłe C1 z pochodnych z beziera dla t=0.5 w brzegu i w popzrednim rzedzie)
        Vect3 crossDeriv_prev_1 = R[prev][2] - IR[prev][2];
        Vect3 crossDeriv_prev_2 = R[prev][1] - IR[prev][1];
        Vect3 P11u_pos = R[prev][2] + crossDeriv_prev_1;
        Vect3 P12u_pos = R[prev][1] + crossDeriv_prev_2;

        Vect3 crossDeriv_curr_1 = L[curr][1] - IL[curr][1];
        Vect3 crossDeriv_curr_2 = L[curr][2] - IL[curr][2];
        Vect3 P11v_pos = L[curr][1] + crossDeriv_curr_1;
        Vect3 P21v_pos = L[curr][2] + crossDeriv_curr_2;

        Vect3 P21u_pos = P21v_pos;
        Vect3 P12v_pos = P12u_pos;

        // wąsy wychodzace z rad[2]
        Vect3 E_curr = Rad[curr][2] - Pc_pos;
        Vect3 E_prev = Rad[prev][2] - Pc_pos;


        Vect3 P22u_pos = Rad[curr][2] + E_prev * (2.0f / 3.0f) + E_curr * (1.0f / 3.0f);
        Vect3 P22v_pos = Rad[prev][2] + E_curr * (2.0f / 3.0f) + E_prev * (1.0f / 3.0f);


        auto p11u = std::make_shared<ScenePoint>("p11u_"+std::to_string(i), P11u_pos);
        p11u->Init();
        //sceneObjects.push_back(p11u);
        auto p11v = std::make_shared<ScenePoint>("p11v_"+std::to_string(i), P11v_pos);
        p11v->Init();
        //sceneObjects.push_back(p11v);

        auto p12u = std::make_shared<ScenePoint>("p12u_"+std::to_string(i), P12u_pos);
        p12u->Init();
        //sceneObjects.push_back(p12u);
        auto p12v = std::make_shared<ScenePoint>("p12v_"+std::to_string(i), P12v_pos);
        p12v->Init();
        //sceneObjects.push_back(p12v);

        auto p21u = std::make_shared<ScenePoint>("p21u_"+std::to_string(i), P21u_pos);
        p21u->Init();
        //sceneObjects.push_back(p21u);
        auto p21v = std::make_shared<ScenePoint>("p21v_"+std::to_string(i), P21v_pos);
        p21v->Init();
        //sceneObjects.push_back(p21v);

        auto p22u = std::make_shared<ScenePoint>("p22u_"+std::to_string(i), P22u_pos);
        p22u->Init();
        //sceneObjects.push_back(p22u);
        auto p22v = std::make_shared<ScenePoint>("p22v_"+std::to_string(i), P22v_pos);
        p22v->Init();
        //sceneObjects.push_back(p22v);


        patch->points[offset + 0] = bnd_V0[0];
        patch->points[offset + 1] = bnd_V0[1];
        patch->points[offset + 2] = bnd_V0[2];
        patch->points[offset + 3] = bnd_V0[3];

        patch->points[offset + 4] = bnd_U0[1];
        patch->points[offset + 5] = p11u;
        patch->points[offset + 6] = p11v;
        patch->points[offset + 7] = p21u;
        patch->points[offset + 8] = p21v;
        patch->points[offset + 9] = bnd_U1[1];

        patch->points[offset + 10]= bnd_U0[2];
        patch->points[offset + 11]= p12u;
        patch->points[offset + 12]= p12v;
        patch->points[offset + 13]= p22u;
        patch->points[offset + 14]= p22v;
        patch->points[offset + 15]= bnd_U1[2];

        patch->points[offset + 16]= bnd_V1[0];
        patch->points[offset + 17]= bnd_V1[1];
        patch->points[offset + 18]= bnd_V1[2];
        patch->points[offset + 19]= bnd_V1[3];
    }

    for(auto& wp : patch->bezierPatchPoints)
        if(auto p = wp.lock())
            p->globalSurfacesCount++;

    for(auto& wp : patch->bezierPatchPointsInner)
        if(auto p = wp.lock())
            p->globalSurfacesCount++;

    patch->Init();
    return patch;
}







void UpdateGregoryPositions(const std::weak_ptr<SceneGregoryPatch>& patch, const PreviewContext &ctx)
{
    // Punkty z Beziera i de Casteljau w t=0.5
    Vect3 E[3][4], IE[3][4];
    Vect3 L[3][4], R[3][4], IL[3][4], IR[3][4];

    auto patch_ptr = patch.lock();
    for (int i = 0; i < 3; ++i)
    {
        for(int j=0; j<4; ++j)
        {
            auto p = patch_ptr->bezierPatchPoints[i * 4 + j].lock();
            E[i][j] = getPreviewPosition(p, ctx);//p->transformations.getPosition();


            auto p_inner = patch_ptr->bezierPatchPointsInner[i * 4 + j].lock();
            IE[i][j] = getPreviewPosition(p_inner, ctx); //p_inner->transformations.getPosition();
        }

        SubdivideBezier(E[i], L[i], R[i]);
        SubdivideBezier(IE[i], IL[i], IR[i]);
    }


    // Wyliczanie szkieletu
    Vect3 Rad[3][4];
    //std::shared_ptr<ScenePoint> pt_Rad[3][4];
    Vect3 Q[3]; // Punkty pomocnicze Qi

    for (int i = 0; i < 3; ++i)
    {
        Vect3 E_pt = L[i][3];  // P_3i z obrazka
        Vect3 I_pt = IL[i][3];

        Rad[i][0] = E_pt; // Midpoint z beziera t = 0.5

        // P_2i z obrazka (warunek C1)
        Rad[i][1] = E_pt + (E_pt - I_pt);

        // Qi = P_3i + 3 * (P_2i - P_3i) / 2
        Q[i] = Rad[i][0] + (Rad[i][1] - Rad[i][0]) * 1.5f;
    }

    // Punkt Centralny P (Pc) <=> środek ciężkości punktów Q_i
    Vect3 Pc_pos = (Q[0] + Q[1] + Q[2]) / 3.0f;

    for (int i = 0; i < 3; ++i)
    {
        // Wyliczamy P_1i (nasze Rad[2]) ze wzoru na obrazku: P_1i = P + 2/3 * (Q_i - P)
        Rad[i][2] = Pc_pos + (Q[i] - Pc_pos) * (2.0f / 3.0f);
        Rad[i][3] = Pc_pos;
    }







    //Składanie 3 sub-płatów i punkty wewnętrzne
    for (int i = 0; i < 3; ++i)
    {
        int prev = (i + 2) % 3;
        int curr = i;
        int offset = i * 20;


        // Zewnętrzne krawędzie (Ścisłe C1 z pochodnych z beziera dla t=0.5 w brzegu i w popzrednim rzedzie)
        Vect3 crossDeriv_prev_1 = R[prev][2] - IR[prev][2];
        Vect3 crossDeriv_prev_2 = R[prev][1] - IR[prev][1];
        Vect3 P11u_pos = R[prev][2] + crossDeriv_prev_1;
        Vect3 P12u_pos = R[prev][1] + crossDeriv_prev_2;

        Vect3 crossDeriv_curr_1 = L[curr][1] - IL[curr][1];
        Vect3 crossDeriv_curr_2 = L[curr][2] - IL[curr][2];
        Vect3 P11v_pos = L[curr][1] + crossDeriv_curr_1;
        Vect3 P21v_pos = L[curr][2] + crossDeriv_curr_2;

        Vect3 P21u_pos = P21v_pos;
        Vect3 P12v_pos = P12u_pos;

        // wąsy wychodzace z rad[2]
        Vect3 E_curr = Rad[curr][2] - Pc_pos;
        Vect3 E_prev = Rad[prev][2] - Pc_pos;


        Vect3 P22u_pos = Rad[curr][2] + E_prev * (2.0f / 3.0f) + E_curr * (1.0f / 3.0f);
        Vect3 P22v_pos = Rad[prev][2] + E_curr * (2.0f / 3.0f) + E_prev * (1.0f / 3.0f);


        Vect3 positions[20];
        positions[0] = L[curr][0];
        positions[1] = L[curr][1];
        positions[2] = L[curr][2];
        positions[3] = L[curr][3];

        positions[4] =  R[prev][2];
        positions[5] = P11u_pos;
        positions[6] = P11v_pos;
        positions[7] = P21u_pos;
        positions[8] = P21v_pos;
        positions[9] =  Rad[curr][1];

        positions[10] = R[prev][1];
        positions[11] = P12u_pos;
        positions[12] = P12v_pos;
        positions[13] = P22u_pos;
        positions[14] = P22v_pos;
        positions[15] =  Rad[curr][2];

        positions[16] = Rad[prev][0];
        positions[17] = Rad[prev][1];
        positions[18] = Rad[prev][2];
        positions[19] = Rad[prev][3];


        for(int j = 0; j< 20; j++)
        {
            auto p = patch_ptr->points[offset + j];
            p->transformations.setPosition(positions[j]);
        }


    }

}