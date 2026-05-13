#include "sceneSerializer.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <nlohmann/json.hpp> // Wymaga wrzucenia json.hpp do external/nlohmann/

// Twoje nagłówki obiektów
#include "scenePoint.h"
#include "sceneTorus.h"
#include "sceneBezierC0.h"
#include "sceneBezierC2.h"
#include "sceneSplineInterpolating.h"
#include "sceneSurface.h"

using json = nlohmann::json;

void SceneSerializer::LoadScene(const std::string& filepath, std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Nie mozna otworzyc pliku: " << filepath << std::endl;
        return;
    }

    json j;
    try {
        file >> j;
    } catch (json::parse_error& e) {
        std::cerr << "Blad parsowania JSON: " << e.what() << std::endl;
        return;
    }

    // Mapa ułatwiająca wyszukiwanie wczytanych punktów po ich JSON-owym 'id'
    std::unordered_map<int, std::shared_ptr<ScenePoint>> pointMap;

    // ==========================================
    // 1. WCZYTYWANIE PUNKTÓW (POINTS)
    // ==========================================
    if (j.contains("points"))
    {
        for (const auto& pJson : j["points"])
        {
            int id = pJson["id"];
            std::string name = pJson.value("name", "Point " + std::to_string(id));

            float x = pJson["position"]["x"];
            float y = pJson["position"]["y"];
            float z = pJson["position"]["z"];

            Transformations t;
            t.setPosition(Vect3(x, y, z));

            auto point = std::make_shared<ScenePoint>(name, t);
            point->Init();

            pointMap[id] = point;
            sceneObjects.push_back(point);
        }
    }

    // ==========================================
    // 2. WCZYTYWANIE GEOMETRII (GEOMETRY)
    // ==========================================
    if (j.contains("geometry"))
    {
        for (const auto& gJson : j["geometry"])
        {
            std::string type = gJson["objectType"];
            int id = gJson["id"];
            std::string name = gJson.value("name", type + " " + std::to_string(id));

            // Pomocnicza lambda: Wyciąga fizyczne wskaźniki na punkty na podstawie listy JSON {"id": ...}
            auto getControlPoints = [&]() {
                std::vector<std::shared_ptr<ScenePoint>> pts;
                if (gJson.contains("controlPoints"))
                {
                    for (const auto& cpJson : gJson["controlPoints"])
                    {
                        int cpId = cpJson["id"];
                        if (pointMap.count(cpId))
                        {
                            pts.push_back(pointMap[cpId]);
                        }
                    }
                }
                return pts;
            };

            // ---- TORUS ----
            if (type == "torus")
            {
                Transformations t;
                t.posX = gJson["position"]["x"];
                t.posY = gJson["position"]["y"];
                t.posZ = gJson["position"]["z"];

                // Torus w formacie ma "scale" jako float3, ale my używamy stałego float. Bierzemy X.
                t.scale = gJson["scale"]["x"];

                t.rotation.w = gJson["rotation"]["w"];
                t.rotation.x = gJson["rotation"]["x"];
                t.rotation.y = gJson["rotation"]["y"];
                t.rotation.z = gJson["rotation"]["z"];

                auto torus = std::make_shared<SceneTorus>(name, t);
                torus->R = gJson["largeRadius"];
                torus->r = gJson["smallRadius"];
                torus->density_R = gJson["samples"]["u"];
                torus->density_r = gJson["samples"]["v"];
                torus->Init();

                sceneObjects.push_back(torus);
            }

                // ---- KRZYWE (BEZIER C0, C2, SPLAJN INTERPOLACYJNY) ----
            else if (type == "bezierC0" || type == "bezierC2" || type == "interpolatedC2")
            {
                std::shared_ptr<SceneBezier> curve;

                if (type == "bezierC0")
                    curve = std::make_shared<SceneBezierC0>(name, Transformations());
                else if (type == "bezierC2")
                    curve = std::make_shared<SceneBezierC2>(name, Transformations());
                else
                    curve = std::make_shared<SceneSplineInterpolating>(name, Transformations());

                curve->Init();
                // Ponieważ ignorujemy "chain", wymuszamy, by domyślnie wyświetlały swoje wieloboki:
                curve->showPolygon = true;

                for (auto& p : getControlPoints())
                {
                    curve->points.push_back(p);
                    p->globalCurvesCount++; // Aktualizacja flagi z KROKU 1!
                }

                sceneObjects.push_back(curve);
            }

                // ---- POWIERZCHNIE (PŁATY BIKUBICZNE C0, C2) ----
            else if (type == "bezierSurfaceC0" || type == "bezierSurfaceC2")
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
                surface->showPolygon = true;

                for (auto& p : getControlPoints())
                {
                    surface->points.push_back(p);
                    p->belongsToPatch = true; // Zabezpieczenie przed usunięciem punktu!
                }

                sceneObjects.push_back(surface);
            }

                // ---- ŁAMANA (CHAIN) ORAZ INNE ----
            else if (type == "chain")
            {
                // Zgodnie z wytycznymi - ignorujemy obiekt chain.
                continue;
            }
        }
    }
}