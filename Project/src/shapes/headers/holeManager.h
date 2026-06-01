#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include "scenePoint.h"
#include "sceneSurface.h"

// 1. Struktura krawędzi (Mały segment = 4 punkty)
struct HoleSegment {
    int edgeId;
    std::shared_ptr<ScenePoint> p[4];
    std::shared_ptr<SceneSurfaceC0> owner;

    HoleSegment reversed() const {
        return {edgeId, {p[3], p[2], p[1], p[0]}, owner};
    }

    bool isEquivalent(const HoleSegment& other) const {
        bool fwd = (p[0] == other.p[0] && p[1] == other.p[1] && p[2] == other.p[2] && p[3] == other.p[3]);
        bool rev = (p[0] == other.p[3] && p[1] == other.p[2] && p[2] == other.p[1] && p[3] == other.p[0]);
        return fwd || rev;
    }
};

// 2. Struktura cyklu o DOWOLNEJ długości (ZMIANA NA VECTOR)
struct HoleCycle {
    std::vector<HoleSegment> edges;
};

// 3. Reprezentacja krawędzi w grafie
struct GraphEdge {
    int edgeId;
    std::shared_ptr<ScenePoint> targetNode;
    HoleSegment data;
};

using TopologyGraph = std::unordered_map<std::shared_ptr<ScenePoint>, std::vector<GraphEdge>>;


// ----------------------------------------------------------------------------------
// Algorytm Ograniczonego DFS z Nawrotami (Szukanie cykli prostych o DOWOLNEJ długości)
// ----------------------------------------------------------------------------------
inline void BoundedDFS(
        std::shared_ptr<ScenePoint> current,
        std::shared_ptr<ScenePoint> start,
        std::vector<HoleSegment>& path,
        std::unordered_set<std::shared_ptr<ScenePoint>>& visitedNodes,
        std::unordered_set<int>& visitedEdges,
        TopologyGraph& graph,
        int maxDepth,
        std::set<std::set<int>>& uniqueCycles,
        std::vector<std::vector<HoleSegment>>& allCycles)
{
    // =========================================================
    // TWOJ PRZEŁĄCZNIK TESTOWY:
    // true  -> szuka tylko dziur o długości 3 (zgodnie z PDF)
    // false -> szuka dziur o DOWOLNEJ długości (1, 2, 3, 4, 5...)
    // =========================================================
    const bool ONLY_3_SIDED = true;

    // WARUNEK SUKCESU: Ścieżka wróciła do wierzchołka startowego
    if (!path.empty() && current == start)
    {
        // FILTR: Jeśli tryb ONLY_3_SIDED jest włączony, a ścieżka nie ma 3 krawędzi -> odrzucamy!
        if (ONLY_3_SIDED && path.size() != 3)
        {
            return;
        }

        // Zapisujemy cykl (bo albo ma długość 3, albo tryb ONLY_3_SIDED jest wyłączony)
        std::set<int> cycleSignature;
        for (const auto& seg : path)
            cycleSignature.insert(seg.edgeId);

        if (uniqueCycles.find(cycleSignature) == uniqueCycles.end()) {
            uniqueCycles.insert(cycleSignature);
            allCycles.push_back(path);
        }
        return; // Zamykamy tę odnogę rekurencji
    }

    // WARUNEK PRZERWANIA: Osiągnęliśmy limit głębokości
    if (path.size() >= maxDepth)
        return;

    for (const auto& edge : graph[current])
    {
        // 1. Zabezpieczenie Multigrafu: Nie idź dwa razy po tej samej krawędzi
        if (visitedEdges.count(edge.edgeId))
            continue;

        // 2. Zabezpieczenie Cyklu Prostego (Brak "ósemek"):
        // Wolno nam wejść drugi raz TYLKO do węzła startowego!
        if (edge.targetNode != start && visitedNodes.count(edge.targetNode))
            continue;

        // --- KROK DO PRZODU ---
        visitedEdges.insert(edge.edgeId);
        if (edge.targetNode != start)
            visitedNodes.insert(edge.targetNode);
        path.push_back(edge.data);

        BoundedDFS(edge.targetNode, start, path, visitedNodes, visitedEdges, graph, maxDepth, uniqueCycles, allCycles);

        // --- KROK DO TYŁU (BACKTRACKING) ---
        path.pop_back();
        if (edge.targetNode != start)
            visitedNodes.erase(edge.targetNode);
        visitedEdges.erase(edge.edgeId);
    }
}


// ----------------------------------------------------------------------------------
// Główna funkcja wywoływana przez interfejs (Zmieniona nazwa dla precyzji)
// ----------------------------------------------------------------------------------
inline std::vector<HoleCycle> FindHoles(const std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    std::vector<HoleSegment> allEdges;

    // ETAP 1: Zbieranie wszystkich krawędzi
    for (const auto& obj : sceneObjects)
    {
        if (obj->isSelected && obj->objectType == ObjectType::BezierSurfaceC0)
        {
            auto s = std::static_pointer_cast<SceneSurfaceC0>(obj);
            int pU = (s->sizeU - 1) / 3;
            int pV = (s->sizeV - 1) / 3;

            auto getPt = [&](int u, int v) { return s->points[v * s->sizeU + u].lock(); };

            auto addEdge = [&](int u1, int v1, int u2, int v2, int u3, int v3, int u4, int v4) {
                auto pt0 = getPt(u1, v1), pt1 = getPt(u2, v2), pt2 = getPt(u3, v3), pt3 = getPt(u4, v4);

                // Ignorujemy puste oraz CAŁKOWITĄ DEGENERACJĘ (krawędź zgnieciona do 1 punktu)
                if (pt0 && pt1 && pt2 && pt3) {
                    if (!(pt0 == pt1 && pt1 == pt2 && pt2 == pt3)) {
                        allEdges.push_back({-1, {pt0, pt1, pt2, pt3}, s});
                    }
                }
            };

            for(int i = 0; i < pU; ++i) addEdge(3*i, 0, 3*i+1, 0, 3*i+2, 0, 3*i+3, 0);
            for(int i = 0; i < pU; ++i) addEdge(3*i, s->sizeV-1, 3*i+1, s->sizeV-1, 3*i+2, s->sizeV-1, 3*i+3, s->sizeV-1);
            for(int j = 0; j < pV; ++j) addEdge(0, 3*j, 0, 3*j+1, 0, 3*j+2, 0, 3*j+3);
            for(int j = 0; j < pV; ++j) addEdge(s->sizeU-1, 3*j, s->sizeU-1, 3*j+1, s->sizeU-1, 3*j+2, s->sizeU-1, 3*j+3);
        }
    }

    // ETAP 2: Odrzucanie szwów
    std::vector<HoleSegment> boundaryEdges;
    int edgeCounter = 0;
    for (size_t i = 0; i < allEdges.size(); ++i)
    {
        int occurrences = 0;
        for (size_t j = 0; j < allEdges.size(); ++j) {
            if (allEdges[i].isEquivalent(allEdges[j])) occurrences++;
        }

        if (occurrences == 1) {
            HoleSegment edge = allEdges[i];
            edge.edgeId = edgeCounter++;
            boundaryEdges.push_back(edge);
        }
    }

    // ETAP 3: Budowa Grafu W Locie
    TopologyGraph graph;
    for (const auto& edge : boundaryEdges)
    {
        auto nodeA = edge.p[0];
        auto nodeB = edge.p[3];

        graph[nodeA].push_back({edge.edgeId, nodeB, edge});
        graph[nodeB].push_back({edge.edgeId, nodeA, edge.reversed()});
    }

    // ETAP 4: Odpalenie DFS
    std::set<std::set<int>> uniqueCycles;
    std::vector<std::vector<HoleSegment>> allCycles;

    // Maksymalna głębokość to ilość wszystkich krawędzi, bo cykl nie może być dłuższy!
    int maxCycleLength = boundaryEdges.size();

    for (const auto& pair : graph)
    {
        auto startNode = pair.first;
        std::vector<HoleSegment> path;
        std::unordered_set<std::shared_ptr<ScenePoint>> visitedNodes;
        std::unordered_set<int> visitedEdges;

        visitedNodes.insert(startNode);
        BoundedDFS(startNode, startNode, path, visitedNodes, visitedEdges, graph, maxCycleLength, uniqueCycles, allCycles);
    }

    // ETAP 5: Zwrócenie ustrukturyzowanych wyników
    std::vector<HoleCycle> results;
    for (const auto& cyclePath : allCycles)
    {
        HoleCycle hc;
        hc.edges = cyclePath; // Dynamiczna długość
        results.push_back(hc);
    }

    return results;
}


// Pobieranie wewnętrznego rzędu płata Beziera
inline void GetInnerRow(const HoleSegment& edge, Vect3 I[4])
{
    auto s = edge.owner;
    int sizeU = s->sizeU, sizeV = s->sizeV;
    int u0 = -1, v0 = -1, du = 0, dv = 0;

    for (int v = 0; v < sizeV; ++v) {
        for (int u = 0; u < sizeU; ++u) {
            if (s->points[v * sizeU + u].lock() == edge.p[0]) {
                if (u + 1 < sizeU && s->points[v * sizeU + u + 1].lock() == edge.p[1]) { u0=u; v0=v; du=1; dv=0; break; }
                if (u - 1 >= 0 && s->points[v * sizeU + u - 1].lock() == edge.p[1]) { u0=u; v0=v; du=-1; dv=0; break; }
                if (v + 1 < sizeV && s->points[(v + 1) * sizeU + u].lock() == edge.p[1]) { u0=u; v0=v; du=0; dv=1; break; }
                if (v - 1 >= 0 && s->points[(v - 1) * sizeU + u].lock() == edge.p[1]) { u0=u; v0=v; du=0; dv=-1; break; }
            }
        }
        if (du != 0 || dv != 0) break;
    }

    int inner_du = 0, inner_dv = 0;
    if (du != 0) inner_dv = (v0 == 0) ? 1 : -1;
    else         inner_du = (u0 == 0) ? 1 : -1;

    for (int k = 0; k < 4; ++k) {
        int u_I = u0 + k * du + inner_du;
        int v_I = v0 + k * dv + inner_dv;
        if (u_I >= 0 && u_I < sizeU && v_I >= 0 && v_I < sizeV) {
            if (auto pt = s->points[v_I * sizeU + u_I].lock()) I[k] = pt->transformations.getPosition();
            else I[k] = edge.p[k]->transformations.getPosition();
        } else I[k] = edge.p[k]->transformations.getPosition();
    }
}