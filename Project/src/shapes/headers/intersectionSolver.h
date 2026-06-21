#pragma once
#include "evaluableSurface.h"
#include <MG1Math/Vect3.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <optional>

struct IntersectionPoint {
    Vect3 worldPos;
    float uA, vA;
    float uB, vB;
};

class IntersectionSolver {
public:

    // rzutowanie punktu target na powierzchnie (Gauss-Newton)
    template<EvaluableSurface S>
    static bool ProjectPointToSurface(const S& surf, const Vect3& target, float& u, float& v)
    {
        const int MAX_ITER = 15;

        for (int i = 0; i < MAX_ITER; ++i)
        {
            Vect3 P = surf.EvaluatePos(u, v);
            Vect3 Pu = surf.EvaluateDu(u, v);
            Vect3 Pv = surf.EvaluateDv(u, v);
            Vect3 diff = P - target;

            float f1 = Vect3::dot(diff, Pu);
            float f2 = Vect3::dot(diff, Pv);

            float J11 = Vect3::dot(Pu, Pu);
            float J12 = Vect3::dot(Pu, Pv);
            float J21 = J12;
            float J22 = Vect3::dot(Pv, Pv);

            float det = J11 * J22 - J12 * J21;
            if (std::abs(det) < 1e-8f)
                return false;

            float invDet = 1.0f / det;
            float du = -( J22 * f1 - J12 * f2) * invDet;
            float dv = -(-J21 * f1 + J11 * f2) * invDet;

            // Zamiast patrzeć na iloczyn skalarny, patrzymy czy parametry przestały się zmieniać
            if (std::abs(du) < 1e-6f && std::abs(dv) < 1e-6f)
                return true;

            u += du;
            v += dv;

            if (surf.isWrappedU())
            {
                while (u < 0.0f)
                    u += 1.0f;

                while (u > 1.0f)
                    u -= 1.0f;
            }
            else
                u = std::clamp(u, 0.0f, 1.0f);

            if (surf.isWrappedV())
            {
                while
                (v < 0.0f)
                    v += 1.0f;
                while (v > 1.0f)
                    v -= 1.0f;
            }
            else
                v = std::clamp(v, 0.0f, 1.0f);
        }
        return true;
    }

    //(Hybrydowy Hunting + Bidirectional Marching)
    template<EvaluableSurface SurfA, EvaluableSurface SurfB>
    static std::vector<IntersectionPoint> FindIntersection(const SurfA& A, const SurfB& B,
                                                           float stepSize = 0.05f, bool useCursor = false, Vect3 cursorPos = Vect3(0,0,0),
                                                           bool isSelfIntersect = false)
    {
        float start_uA = 0.5f, start_vA = 0.5f, start_uB = 0.5f, start_vB = 0.5f;


        if (useCursor && !isSelfIntersect)
        {
            // Zwykłe rzutowanie dla 2 różnych powierzchni
            ProjectPointToSurface(A, cursorPos, start_uA, start_vA);
            ProjectPointToSurface(B, cursorPos, start_uB, start_vB);
        }
        else if (useCursor && isSelfIntersect)
        {
            // pkt A z kursora
            ProjectPointToSurface(A, cursorPos, start_uA, start_vA);

            //pkt B z losowania - w UV ma byc daleko od A, ale w świecie ma byc blisko (mały distance)
            float bestDistSq = 1e20f;
            const int SAMPLES = 25;
            for (int k = 0; k <= SAMPLES; ++k)
            {
                float s = (float)k / SAMPLES;
                for (int l = 0; l <= SAMPLES; ++l)
                {
                    float t = (float)l / SAMPLES;

                    float du = std::abs(start_uA - s);
                    if (A.isWrappedU() && du > 0.5f) //bo i tak zawijamy czyli odleglośc 0. w U lub V jest maxymalna
                        du = 1.0f - du;

                    float dv = std::abs(start_vA - t);
                    if (A.isWrappedV() && dv > 0.5f)
                        dv = 1.0f - dv;

                    // Szukamy punktu daleko od pierwszego w UV (zabezpieczenie)
                    if (du * du + dv * dv < 0.05f)
                        continue;

                    Vect3 pB = B.EvaluatePos(s, t);
                    Vect3 diff = pB - cursorPos;
                    float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
                    if (distSq < bestDistSq)
                    {
                        bestDistSq = distSq;
                        start_uB = s;
                        start_vB = t;
                    }
                }
            }

            ProjectPointToSurface(B, cursorPos, start_uB, start_vB);
        }
        else
        {
            // z całej siatki
            float bestDistSq = 9999999.0f;
            const int SAMPLES = 25;
            for (int i = 0; i <= SAMPLES; ++i)
            {
                for (int j = 0; j <= SAMPLES; ++j)
                {
                    float u = (float)i / SAMPLES;
                    float v = (float)j / SAMPLES;
                    Vect3 pA = A.EvaluatePos(u, v);
                    for(int k=0; k <= SAMPLES; k+=3)
                    {
                        for(int l=0; l <= SAMPLES; l+=3)
                        {
                            float s = (float)k/SAMPLES;
                            float t = (float)l/SAMPLES;

                            if (isSelfIntersect) //zeby wspołrzedne UV były daleko od sibie
                            {
                                float du = std::abs(u - s);
                                if (A.isWrappedU() && du > 0.5f)
                                    du = 1.0f - du;

                                float dv = std::abs(v - t);
                                if (A.isWrappedV() && dv > 0.5f)
                                    dv = 1.0f - dv;

                                if (du * du + dv * dv < 0.05f)
                                    continue;
                            }

                            Vect3 pB = B.EvaluatePos(s, t);
                            Vect3 diff = pA - pB;
                            float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
                            if (distSq < bestDistSq)
                            {
                                bestDistSq = distSq;
                                start_uA = u; start_vA = v;
                                start_uB = s; start_vB = t;
                            }
                        }
                    }
                }
            }
        }

        // MOCNY PING-PONG (do 50 razy, likwiduje "wzgórze" na początku krzywej)
        for (int k = 0; k < 50; ++k)
        {
            Vect3 pA = A.EvaluatePos(start_uA, start_vA);
            ProjectPointToSurface(B, pA, start_uB, start_vB);

            Vect3 pB = B.EvaluatePos(start_uB, start_vB);
            ProjectPointToSurface(A, pB, start_uA, start_vA);

            Vect3 diff = A.EvaluatePos(start_uA, start_vA) - B.EvaluatePos(start_uB, start_vB);
            if (diff.x*diff.x + diff.y*diff.y + diff.z*diff.z < 1e-8f)
            {
                break; // Znaleziono perfekcyjny styk!
            }
        }

        //czy newton sie udał
        Vect3 diff = A.EvaluatePos(start_uA, start_vA) - B.EvaluatePos(start_uB, start_vB);
        if (std::sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z) > 0.5f)
        {
            return std::vector<IntersectionPoint>();
        }

        // maszerujemy
        auto doMarch = [&](float dirSign, bool& isClosed)
        {
            std::vector<IntersectionPoint> subCurve;
            float curr_uA = start_uA, curr_vA = start_vA;
            float curr_uB = start_uB, curr_vB = start_vB;

            IntersectionPoint firstPt = {A.EvaluatePos(curr_uA, curr_vA), curr_uA, curr_vA, curr_uB, curr_vB};
            subCurve.push_back(firstPt);
            isClosed = false;

            Vect3 previousTangent(0.0f);

            const int MAX_STEPS = 500 * static_cast<int>(1.0f / stepSize);
            for (int step = 0; step < MAX_STEPS; ++step)
            {
                Vect3 normA = Vect3::cross(A.EvaluateDu(curr_uA, curr_vA), A.EvaluateDv(curr_uA, curr_vA)).normalize();
                Vect3 normB = Vect3::cross(B.EvaluateDu(curr_uB, curr_vB), B.EvaluateDv(curr_uB, curr_vB)).normalize();

                Vect3 tangent = Vect3::cross(normA, normB); //kierunek poruszania sie
                if (tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z < 0.0001f)
                    break;
                tangent.normalize();

                if (step == 0)
                {
                    tangent *= dirSign; //z gory narzucony kietunek
                }
                else
                {
                    if (Vect3::dot(tangent, previousTangent) < 0.0f) //czy sie losowo nie obrocolismy o 180 stopni
                    {
                        tangent *= -1.0f;
                    }
                }
                previousTangent = tangent;

                // wzdłuz wektora tangent o step_size idziemy
                Vect3 candX = subCurve.back().worldPos + tangent * stepSize;

                //dociagamy ten punkt do powierzchni A i B i korygujemy jednoczesnie parametry UV
                for (int k = 0; k < 2; ++k)
                {
                    ProjectPointToSurface(A, candX, curr_uA, curr_vA);
                    candX = A.EvaluatePos(curr_uA, curr_vA);
                    ProjectPointToSurface(B, candX, curr_uB, curr_vB);
                    candX = B.EvaluatePos(curr_uB, curr_vB);
                }

                //dla niezakreconych powierzchni
                if (!A.isWrappedU() && (curr_uA <= 0.0001f || curr_uA >= 0.9999f))
                    break;
                if (!A.isWrappedV() && (curr_vA <= 0.0001f || curr_vA >= 0.9999f))
                    break;
                if (!B.isWrappedU() && (curr_uB <= 0.0001f || curr_uB >= 0.9999f))
                    break;
                if (!B.isWrappedV() && (curr_vB <= 0.0001f || curr_vB >= 0.9999f))
                    break;

                //pozycja jako srednia z punktow dla skorygowanych przesunietych paramtrów
                Vect3 finalPos = (A.EvaluatePos(curr_uA, curr_vA) + B.EvaluatePos(curr_uB, curr_vB)) * 0.5f;

                //czy kroczek nie był za malutki (w porownaniu do stepSize)
                Vect3 stepDiff = finalPos - subCurve.back().worldPos;
                if (stepDiff.x*stepDiff.x + stepDiff.y*stepDiff.y + stepDiff.z*stepDiff.z < (stepSize * stepSize * 0.0001f)) break;

                //czy juz zamykamy petle (czyli czy wystarczajaco blisko punktu poczatkowego jesteśmy) - mnijej niz 1.5 naszego kroku
                if (step > 15)
                {
                    Vect3 toStart = finalPos - firstPt.worldPos;
                    if (toStart.x*toStart.x + toStart.y*toStart.y + toStart.z*toStart.z < (stepSize * stepSize * 1.5f))
                    {
                        subCurve.push_back(firstPt);
                        isClosed = true;
                        break;
                    }
                }

                subCurve.push_back({finalPos, curr_uA, curr_vA, curr_uB, curr_vB});
            }
            return subCurve;
        };

        bool isClosedLoop = false;
        // pierwsze puszceznie algorytmu
        std::vector<IntersectionPoint> forwardCurve = doMarch(1.0f, isClosedLoop);

        if (isClosedLoop)
        {
            return forwardCurve;
        }
        else //jezeli nie udało sie nam zamknac petli - albo za mało kroków albo po prostu NIE MA PETLI - to musimy
        // tez iść w drugiem kietunku, bo moglismy Newtonem wylosowac jakis punkts srodkowy krzywej
        {
            bool dummyClosed;
            std::vector<IntersectionPoint> backwardCurve = doMarch(-1.0f, dummyClosed);

            std::vector<IntersectionPoint> fullCurve;
            fullCurve.reserve(backwardCurve.size() + forwardCurve.size());

            // Z backwardCurve bierzemy "od tyłu" (Reverse Iterators - rbegin, rend).
            // Odejmujemy 1 element, żeby celowo zignorować punkt 0 (środek), by go nie dublować.
            if (backwardCurve.size() > 1)
            {
                fullCurve.insert(fullCurve.end(), backwardCurve.rbegin(), backwardCurve.rend() - 1);
            }

            // Doklejamy w całości forwardCurve
            fullCurve.insert(fullCurve.end(), forwardCurve.begin(), forwardCurve.end());

            return fullCurve;
        }
    }
};