#include "sceneSerializer.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "scenePoint.h"
#include "sceneTorus.h"
#include "sceneBezierC0.h"
#include "sceneBezierC2.h"
#include "sceneSplineInterpolating.h"
#include "sceneSurface.h"
#include "sceneGregoryPatch.h" // POTRZEBNE DO RZUTOWANIA I SPRAWDZENIA

using json = nlohmann::json;

// User (Z-up) -> JSON (Y-up)
void mapToJSON(const Vect3& u, float& jx, float& jy, float& jz)
{
    jx = u.x;
    jy = u.z;
    jz = -u.y;
}

// JSON (Y-up) -> User (Z-up)
Vect3 mapToUser(float jx, float jy, float jz)
{
    return Vect3(jx, -jz, jy);
}

void SceneSerializer::LoadScene(const std::string& filepath, std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        return;

    json j;
    try
    {
        file >> j;
    }
    catch (...)
    {
        return;
    }

    std::unordered_map<int, std::shared_ptr<ScenePoint>> pointMap;

    // WCZYTYWANIE PUNKTÓW
    if (j.contains("points"))
    {
        for (const auto& pJson : j["points"])
        {
            int id = pJson["id"];
            Vect3 pos = mapToUser(pJson["position"]["x"], pJson["position"]["y"], pJson["position"]["z"]);

            Transformations t;
            t.setPosition(pos);
            auto point = std::make_shared<ScenePoint>(pJson.value("name", "Point"), t);
            point->Init();

            pointMap[id] = point;
            sceneObjects.push_back(point);
        }
    }

    // 2. WCZYTYWANIE GEOMETRII
    if (j.contains("geometry"))
    {
        for (const auto& gJson : j["geometry"])
        {
            std::string type = gJson["objectType"];
            std::string name = gJson.value("name", type);

            auto getCP = [&](){
                std::vector<std::shared_ptr<ScenePoint>> pts;
                for (const auto& cp : gJson["controlPoints"])
                    if (pointMap.count((int)cp["id"])) pts.push_back(pointMap[(int)cp["id"]]);
                return pts;
            };

            if (type == "torus")
            {
                Transformations t;
                Vect3 pos = mapToUser(gJson["position"]["x"], gJson["position"]["y"], gJson["position"]["z"]);
                t.setPosition(pos);
                t.scale = gJson["scale"]["x"];
                // Mapowanie kwaternionu
                t.rotation.w = gJson["rotation"]["w"];
                t.rotation.x = gJson["rotation"]["x"];
                t.rotation.y = -static_cast<float>(gJson["rotation"]["z"]);
                t.rotation.z = gJson["rotation"]["y"];

                auto torus = std::make_shared<SceneTorus>(name, t);
                torus->R = gJson["largeRadius"];
                torus->r = gJson["smallRadius"];
                torus->density_R = gJson["samples"]["u"];
                torus->density_r = gJson["samples"]["v"];
                torus->Init();
                sceneObjects.push_back(torus);
            }
            else if (type == "bezierC0" ||
                     type == "bezierC2" ||
                     type == "interpolatedC2")
            {
                std::shared_ptr<SceneBezier> curve;
                if (type == "bezierC0")
                    curve = std::make_shared<SceneBezierC0>(name, Transformations());
                else if (type == "bezierC2")
                    curve = std::make_shared<SceneBezierC2>(name, Transformations());
                else
                    curve = std::make_shared<SceneSplineInterpolating>(name, Transformations());

                curve->Init();
                for (auto& p : getCP())
                {
                    curve->points.push_back(p);
                    p->globalCurvesCount++;
                }
                sceneObjects.push_back(curve);
            }
            else if (type == "bezierSurfaceC0" ||
                     type == "bezierSurfaceC2")
            {
                std::shared_ptr<SceneSurface> surface;
                if (type == "bezierSurfaceC0")
                    surface = std::make_shared<SceneSurfaceC0>(name, Transformations());
                else
                    surface = std::make_shared<SceneSurfaceC2>(name, Transformations());

                surface->sizeU = gJson["size"]["u"];
                surface->sizeV = gJson["size"]["v"];
                surface->samplesU = gJson["samples"]["u"];
                surface->samplesV = gJson["samples"]["v"];

                for (auto& p : getCP())
                {
                    surface->points.push_back(p);
                    p->globalSurfacesCount++;
                }

                // Wymóg z readme.md: Sprawdzanie zawinięcia walca po referencjach
                if (surface->sizeU > 1 && surface->sizeV > 0)
                {
                    auto firstPoint = surface->points[0].lock();
                    auto lastPoint = surface->points[surface->sizeU - 1].lock();
                    if (firstPoint && lastPoint && firstPoint == lastPoint)
                    {
                        surface->isCylinder = true;
                    }
                }

                surface->Init();
                sceneObjects.push_back(surface);
            }
        }
    }
}

void SceneSerializer::SaveScene(const std::string& filepath, const std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    json j;
    j["points"] = json::array();
    j["geometry"] = json::array();

    // 0. PRE-PASS: Analiza wystąpień punktów w płatach Gregory'ego
    std::unordered_map<ScenePoint*, int> gregoryUsages;
    for (const auto& obj : sceneObjects)
    {
        if (obj->objectType == ObjectType::GregoryPatch)
        {
            auto greg = std::static_pointer_cast<SceneGregoryPatch>(obj);
            for (const auto& wp : greg->points)
            {
                if (auto p = wp.lock())
                    gregoryUsages[p.get()]++;
            }
        }
    }

    std::unordered_map<ScenePoint*, int> pointToId;
    int pointIdCounter = 1;

    // 1. ZAPIS PUNKTÓW
    for (const auto& obj : sceneObjects)
    {
        if (obj->objectType == ObjectType::Point)
        {
            auto p = std::static_pointer_cast<ScenePoint>(obj);

            // FILTR: Jeśli punkt nie należy do żadnej krzywej, a jego wystąpienia
            // na powierzchniach są zdominowane WYŁĄCZNIE przez płaty Gregory'ego...
            int gCount = gregoryUsages[p.get()];
            if (gCount > 0 && p->globalCurvesCount == 0 && p->globalSurfacesCount == gCount)
            {
                continue; // To sztuczny punkt z wnętrza łaty Gregory'ego -> POMIJAMY
            }

            int id = pointIdCounter++;
            pointToId[p.get()] = id;

            float x, y, z;
            mapToJSON(p->transformations.getPosition(), x, y, z);

            j["points"].push_back({
                                          {"id", id},
                                          {"name", p->name},
                                          {"position", {{"x", x}, {"y", y}, {"z", z}}}
                                  });
        }
    }

    // 2. ZAPIS GEOMETRII
    int geomIdCounter = 1;
    for (const auto& obj : sceneObjects)
    {
        // Omijamy punkty oraz Płaty Gregory'ego (zostaną odtworzone reaktywnie)
        if (obj->objectType == ObjectType::Point || obj->objectType == ObjectType::GregoryPatch)
            continue;

        json g;
        g["id"] = geomIdCounter++;
        g["name"] = obj->name;

        // Pomocnicza do zapisu punktów kontrolnych
        auto writeCP = [&](const std::vector<std::weak_ptr<ScenePoint>>& pts){
            json cpArr = json::array();
            for (auto& wp : pts)
            {
                if (auto p = wp.lock()) {
                    // Zabezpieczenie: zapisz tylko, jeśli punkt przeszedł przez filtry i ma przydzielone ID
                    if (pointToId.find(p.get()) != pointToId.end()) {
                        cpArr.push_back({{"id", pointToId[p.get()]}});
                    }
                }
            }
            return cpArr;
        };

        if (obj->objectType == ObjectType::Torus)
        {
            auto t = std::static_pointer_cast<SceneTorus>(obj);
            g["objectType"] = "torus";
            float x, y, z;
            mapToJSON(t->transformations.getPosition(), x, y, z);
            g["position"] = {{"x", x}, {"y", y}, {"z", z}};
            g["scale"] = {{"x", t->transformations.scale}, {"y", t->transformations.scale}, {"z", t->transformations.scale}};

            // Mapowanie kwaternionu: User(w,x,y,z) -> JSON(w, x, z, -y)
            g["rotation"] ={
                    {"w", t->transformations.rotation.w},
                    {"x", t->transformations.rotation.x},
                    {"y", t->transformations.rotation.z},
                    {"z", -t->transformations.rotation.y}
            };
            g["largeRadius"] = t->R;
            g["smallRadius"] = t->r;
            g["samples"] = {{"u", t->density_R}, {"v", t->density_r}};
        }
        else if (obj->objectType == ObjectType::BezierCurveC0 ||
                 obj->objectType == ObjectType::BezierCurveC2 ||
                 obj->objectType == ObjectType::SplineInterpolating)
        {
            if (obj->objectType == ObjectType::BezierCurveC0)
                g["objectType"] = "bezierC0";
            else if (obj->objectType == ObjectType::BezierCurveC2)
                g["objectType"] = "bezierC2";
            else
                g["objectType"] = "interpolatedC2";

            auto curve = std::static_pointer_cast<SceneBezier>(obj);
            g["controlPoints"] = writeCP(curve->points);
        }
        else if (obj->objectType == ObjectType::BezierSurfaceC0 ||
                 obj->objectType == ObjectType::BezierSurfaceC2)
        {
            g["objectType"] = (obj->objectType == ObjectType::BezierSurfaceC0) ? "bezierSurfaceC0" : "bezierSurfaceC2";
            auto surf = std::static_pointer_cast<SceneSurface>(obj);
            g["controlPoints"] = writeCP(surf->points);
            g["size"] = {{"u", surf->sizeU}, {"v", surf->sizeV}};
            g["samples"] = {{"u", surf->samplesU}, {"v", surf->samplesV}};
        }

        j["geometry"].push_back(g);
    }

    std::ofstream file(filepath);
    if (file.is_open())
        file << j.dump(4);
}