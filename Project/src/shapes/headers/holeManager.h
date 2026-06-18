#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <functional>
#include <algorithm>
#include "scenePoint.h"
#include "sceneSurface.h"

//(Mały segment = 4 punkty)
struct HoleSegment {
    size_t edgeId;
    std::shared_ptr<ScenePoint> p[4];
    std::shared_ptr<SceneSurfaceC0> owner;


    HoleSegment() : edgeId(0), owner(nullptr) {}

    //od razu wylicza uniwersalne ID
    HoleSegment(std::shared_ptr<ScenePoint> p0, std::shared_ptr<ScenePoint> p1,
                std::shared_ptr<ScenePoint> p2, std::shared_ptr<ScenePoint> p3,
                std::shared_ptr<SceneSurfaceC0> own)
    {
        p[0] = p0;
        p[1] = p1;
        p[2] = p2;
        p[3] = p3;
        owner = own;
        edgeId = calculateUniversalId();
    }

    HoleSegment reversed() const {
        HoleSegment rev;
        rev.p[0] = p[3];
        rev.p[1] = p[2];
        rev.p[2] = p[1];
        rev.p[3] = p[0];
        rev.owner = owner;
        rev.edgeId = this->edgeId; // ID niezależnie od kierunku
        return rev;
    }

    // bo id unikalne
    bool isEquivalent(const HoleSegment& other) const {
        return this->edgeId == other.edgeId;
    }

private:
    size_t calculateUniversalId() const {
        unsigned int id0 = p[0]->id;
        unsigned int id1 = p[1]->id;
        unsigned int id2 = p[2]->id;
        unsigned int id3 = p[3]->id;

        // kanoniczny hasz
        // Gwarantuje to, że (P0,P1,P2,P3) da ten sam hash co (P3,P2,P1,P0)
        if (id0 > id3) {
            std::swap(id0, id3);
            std::swap(id1, id2);
        }

        // Łączymy ID Ownera i znormalizowane ID punktów w string
        std::string hashString = std::to_string(owner->id) + "_" +
                                 std::to_string(id0) + "_" +
                                 std::to_string(id1) + "_" +
                                 std::to_string(id2) + "_" +
                                 std::to_string(id3);

        // Zwracamy unikalny Hash
        return std::hash<std::string>{}(hashString);
    }
};

// cykl o DOWOLNEJ długości
struct HoleCycle {
    std::vector<HoleSegment> edges;
    size_t id_gregory = 0; // Uniwersalne ID całej dziury
};


inline size_t CalculateGregoryId(const std::vector<HoleSegment>& edges)
{
    std::vector<size_t> edgeIds;
    for (const auto& edge : edges)
    {
        edgeIds.push_back(edge.edgeId);
    }

    // Sortujemy ID krawędzi : E1->E2->E3 ten sam hash co E3->E1->E2
    std::sort(edgeIds.begin(), edgeIds.end());

    std::string holeHashStr = "";
    for (auto eid : edgeIds)
    {
        holeHashStr += std::to_string(eid) + "_";
    }
    return std::hash<std::string>{}(holeHashStr);
}

// krawędz w grafie
struct GraphEdge{
    size_t edgeId;
    std::shared_ptr<ScenePoint> targetNode;
    HoleSegment data;
};

using TopologyGraph = std::unordered_map<std::shared_ptr<ScenePoint>, std::vector<GraphEdge>>;


// Ograniczony DFS z Nawrotami
inline void BoundedDFS(
        std::shared_ptr<ScenePoint> current,
        std::shared_ptr<ScenePoint> start,
        std::vector<HoleSegment>& path,
        std::unordered_set<std::shared_ptr<ScenePoint>>& visitedNodes,
        std::unordered_set<size_t>& visitedEdges,
        TopologyGraph& graph,
        int maxDepth,
        std::set<std::set<size_t>>& uniqueCycles,
        std::vector<std::vector<HoleSegment>>& allCycles)
{
    const bool ONLY_3_SIDED = true;

    if (!path.empty() && current == start)
    {
        if (ONLY_3_SIDED && path.size() != 3) return;

        std::set<size_t> cycleSignature;
        for (const auto& seg : path)
            cycleSignature.insert(seg.edgeId);

        if (uniqueCycles.find(cycleSignature) == uniqueCycles.end())
        {
            uniqueCycles.insert(cycleSignature);
            allCycles.push_back(path);
        }
        return;
    }

    if (path.size() >= maxDepth)
        return;

    for (const auto& edge : graph[current])
    {
        if (visitedEdges.count(edge.edgeId))
            continue;

        if (edge.targetNode != start && visitedNodes.count(edge.targetNode))
            continue;

        visitedEdges.insert(edge.edgeId);
        if (edge.targetNode != start)
            visitedNodes.insert(edge.targetNode);
        path.push_back(edge.data);

        BoundedDFS(edge.targetNode, start, path, visitedNodes, visitedEdges, graph, maxDepth, uniqueCycles, allCycles);

        path.pop_back();
        if (edge.targetNode != start)
            visitedNodes.erase(edge.targetNode);
        visitedEdges.erase(edge.edgeId);
    }
}


inline std::vector<HoleCycle> FindHoles(const std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    std::vector<HoleSegment> allEdges;

    //Zbieranie wszystkich krawędzi
    for (const auto& obj : sceneObjects)
    {
        if (obj->isSelected && obj->objectType == ObjectType::BezierSurfaceC0)
        {
            auto s = std::static_pointer_cast<SceneSurfaceC0>(obj);
            int pU = (s->sizeU - 1) / 3;
            int pV = (s->sizeV - 1) / 3;

            auto getPt = [&](int u, int v) { return s->points[v * s->sizeU + u].lock(); };

            auto addEdge =
                    [&](int u1, int v1, int u2, int v2, int u3, int v3, int u4, int v4) {
                auto pt0 = getPt(u1, v1),
                     pt1 = getPt(u2, v2),
                     pt2 = getPt(u3, v3),
                     pt3 = getPt(u4, v4);

                if (pt0 && pt1 && pt2 && pt3)
                {
                    if (!(pt0 == pt1 && pt1 == pt2 && pt2 == pt3))
                    {
                        // Segment, który automatycznie liczy swoje globalne, kanoniczne ID
                        allEdges.push_back(HoleSegment(pt0, pt1, pt2, pt3, s));
                    }
                }
            };

            for(int i = 0; i < pU; ++i)
                addEdge(3*i, 0, 3*i+1, 0, 3*i+2, 0, 3*i+3, 0);

            for(int i = 0; i < pU; ++i)
                addEdge(3*i, s->sizeV-1, 3*i+1, s->sizeV-1, 3*i+2, s->sizeV-1, 3*i+3, s->sizeV-1);

            for(int j = 0; j < pV; ++j)
                addEdge(0, 3*j, 0, 3*j+1, 0, 3*j+2, 0, 3*j+3);

            for(int j = 0; j < pV; ++j)
                addEdge(s->sizeU-1, 3*j, s->sizeU-1, 3*j+1, s->sizeU-1, 3*j+2, s->sizeU-1, 3*j+3);

        }
    }

    // 2: Odrzucanie szwów
    std::vector<HoleSegment> boundaryEdges;
    std::unordered_map<size_t, int> edgeOccurrences;

    // Zliczamy wystąpienia po uniwersalnym ID
    for (const auto& edge : allEdges)
    {
        edgeOccurrences[edge.edgeId]++;
    }

    // Zostawiamy tylko te, które wystąpiły 1 raz (brzegowe)
    for (const auto& edge : allEdges)
    {
        if (edgeOccurrences[edge.edgeId] == 1)
        {
            boundaryEdges.push_back(edge);
        }
    }

    // Budowa Grafu
    TopologyGraph graph;
    for (const auto& edge : boundaryEdges)
    {
        auto nodeA = edge.p[0];
        auto nodeB = edge.p[3];

        graph[nodeA].push_back({edge.edgeId, nodeB, edge});
        graph[nodeB].push_back({edge.edgeId, nodeA, edge.reversed()});
    }

    // Odpalenie DFS
    std::set<std::set<size_t>> uniqueCycles;
    std::vector<std::vector<HoleSegment>> allCycles;

    int maxCycleLength = boundaryEdges.size();

    for (const auto& pair : graph)
    {
        auto startNode = pair.first;
        std::vector<HoleSegment> path;
        std::unordered_set<std::shared_ptr<ScenePoint>> visitedNodes;
        std::unordered_set<size_t> visitedEdges;

        visitedNodes.insert(startNode);
        BoundedDFS(startNode, startNode, path, visitedNodes, visitedEdges, graph, maxCycleLength, uniqueCycles, allCycles);
    }

    // Zwrócenie  wyników
    std::vector<HoleCycle> results;
    for (const auto& cyclePath : allCycles)
    {
        HoleCycle hc;
        hc.edges = cyclePath;
        hc.id_gregory = CalculateGregoryId(cyclePath);
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

    for (int v = 0; v < sizeV; ++v)
    {
        for (int u = 0; u < sizeU; ++u)
        {
            if (s->points[v * sizeU + u].lock() == edge.p[0])
            {
                if (u + 1 < sizeU && s->points[v * sizeU + u + 1].lock() == edge.p[1])
                {
                    u0=u;
                    v0=v;
                    du=1;
                    dv=0;
                    break;
                }
                if (u - 1 >= 0 && s->points[v * sizeU + u - 1].lock() == edge.p[1])
                {
                    u0=u;
                    v0=v;
                    du=-1;
                    dv=0;
                    break;
                }
                if (v + 1 < sizeV && s->points[(v + 1) * sizeU + u].lock() == edge.p[1])
                {
                    u0=u;
                    v0=v;
                    du=0;
                    dv=1;
                    break;
                }
                if (v - 1 >= 0 && s->points[(v - 1) * sizeU + u].lock() == edge.p[1])
                {
                    u0=u;
                    v0=v;
                    du=0;
                    dv=-1;
                    break;
                }
            }
        }

        if (du != 0 || dv != 0)
            break;
    }

    int inner_du = 0, inner_dv = 0;
    if (du != 0)
        inner_dv = (v0 == 0) ? 1 : -1;
    else
        inner_du = (u0 == 0) ? 1 : -1;

    for (int k = 0; k < 4; ++k)
    {
        int u_I = u0 + k * du + inner_du;
        int v_I = v0 + k * dv + inner_dv;
        if (u_I >= 0 && u_I < sizeU && v_I >= 0 && v_I < sizeV)
        {
            if (auto pt = s->points[v_I * sizeU + u_I].lock())
                I[k] = pt->transformations.getPosition();
            else
                I[k] = edge.p[k]->transformations.getPosition();
        }
        else
            I[k] = edge.p[k]->transformations.getPosition();
    }
}

