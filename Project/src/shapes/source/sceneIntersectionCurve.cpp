#include "sceneIntersectionCurve.h"
#include <glad/glad.h>
#include <cmath>

SceneIntersectionCurve::SceneIntersectionCurve(std::string n, const std::vector<IntersectionPoint>& pts,
                                               std::shared_ptr<SceneObject> objA, std::shared_ptr<SceneObject> objB,
                                               bool wUA, bool wVA, bool wUB, bool wVB)
        : SceneObject(std::move(n), Transformations(), ObjectType::IntersectionCurve),
          intersectionPoints(pts), objectA(objA), objectB(objB),
          wrapUA(wUA), wrapVA(wVA), wrapUB(wUB), wrapVB(wVB)
{
    color[0] = 0.0f; color[1] = 0.0f; color[2] = 0.0f;
}


SceneIntersectionCurve::~SceneIntersectionCurve()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);

    if (textureA) glDeleteTextures(1, &textureA);
    if (textureB) glDeleteTextures(1, &textureB);
}

void SceneIntersectionCurve::Init()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    std::vector<Vect3> flatPoints;
    for (const auto& p : intersectionPoints)
    {
        flatPoints.push_back(p.worldPos);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, flatPoints.size() * sizeof(Vect3), flatPoints.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vect3), (void*)0);
    glEnableVertexAttribArray(0);

    GenerateParametricTextures();
}

void SceneIntersectionCurve::Draw(Shader& shader)
{
    shader.use();
    Mat4 id(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, id.table);
    glUniform3fv(glGetUniformLocation(shader.ID, "objectColor"), 1, getUpdatedColorToDrawBasedOn(isSelected));

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINE_STRIP, 0, intersectionPoints.size());
}

void SceneIntersectionCurve::Draw(Shader& shader, Mat4 parentMatrix)
{
    Draw(shader); 
}


namespace {
    float pointToSegmentDistance(const Vect3& P, const Vect3& A, const Vect3& B)
    {
        Vect3 AB = B - A;
        Vect3 AP = P - A;
        float L2 = AB.x * AB.x + AB.y * AB.y + AB.z * AB.z;

        if (L2 < 1e-6f) 
        {
            Vect3 diff = P - A;
            return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        }

   
        float t = std::clamp(Vect3::dot(AP, AB) / L2, 0.0f, 1.0f);
        Vect3 proj = A + AB * t;
        Vect3 diff = P - proj;

        return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    }

   
    void RamerDouglasPeucker(const std::vector<IntersectionPoint>& points, float epsilon, size_t start, size_t end, std::vector<bool>& keep) {
        float maxDist = 0.0f;
        size_t index = start;

        
        for (size_t i = start + 1; i < end; ++i)
        {
            float dist = pointToSegmentDistance(points[i].worldPos, points[start].worldPos, points[end].worldPos);
            if (dist > maxDist)
            {
                maxDist = dist;
                index = i;
            }
        }

        if (maxDist > epsilon)
        {
            keep[index] = true;
            RamerDouglasPeucker(points, epsilon, start, index, keep);
            RamerDouglasPeucker(points, epsilon, index, end, keep);
        }
    }
}


std::shared_ptr<SceneSplineInterpolating> SceneIntersectionCurve::convertToSpline(std::vector<std::shared_ptr<SceneObject>>& sceneObjects, float tolerance)
{
    auto spline = std::make_shared<SceneSplineInterpolating>(name + " (Spline)", Transformations());
    spline->Init();

    if (intersectionPoints.size() < 2)
        return spline;

    std::vector<bool> keepPoint(intersectionPoints.size(), false);

    keepPoint.front() = true;
    keepPoint.back() = true;

    RamerDouglasPeucker(intersectionPoints, tolerance, 0, intersectionPoints.size() - 1, keepPoint);


    for (size_t i = 0; i < intersectionPoints.size(); ++i)
    {
        if (keepPoint[i])
        {
            auto pt = std::make_shared<ScenePoint>("P_int_" + std::to_string(i), intersectionPoints[i].worldPos);
            pt->Init();
            sceneObjects.push_back(pt);

            spline->points.push_back(pt);
            pt->globalCurvesCount++;
        }
    }

    return spline;
}


void SceneIntersectionCurve::GenerateParametricTextures()
{
    const int res = 2048;
    std::vector<unsigned char> bufferA(res * res * 4, 255);
    std::vector<unsigned char> bufferB(res * res * 4, 255);


    auto drawThickPixel = [&](std::vector<unsigned char>& buf, int cx, int cy) {
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int x = ((cx + i) % res + res) % res;
                int y = ((cy + j) % res + res) % res;
                int idx = (y * res + x) * 4;
                buf[idx] = 0;
                buf[idx+1] = 0;
                buf[idx+2] = 0;
                buf[idx+3] = 255;
            }
        }
    };

    // Bresenham
    auto drawLine = [&](std::vector<unsigned char>& buf, int x0, int y0, int x1, int y1) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;
        for (;;)
        {
            drawThickPixel(buf, x0, y0);
            if (x0 == x1 && y0 == y1)
                break;

            e2 = 2 * err;
            if (e2 >= dy)
            {
                err += dy;
                x0 += sx;
            }

            if (e2 <= dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    };

    if (intersectionPoints.size() > 1)
    {
        for (size_t i = 0; i < intersectionPoints.size() - 1; ++i)
        {

            // surf A
            int xA0 = (int)(intersectionPoints[i].uA * res);
            xA0 = (xA0 % res + res) % res;

            int yA0 = (int)(intersectionPoints[i].vA * res);
            yA0 = (yA0 % res + res) % res;

            int xA1 = (int)(intersectionPoints[i+1].uA * res);
            xA1 = (xA1 % res + res) % res;

            int yA1 = (int)(intersectionPoints[i+1].vA * res);
            yA1 = (yA1 % res + res) % res;

            
            if (xA1 - xA0 > res / 2)
                xA1 -= res;
            else if (xA0 - xA1 > res / 2)
                xA1 += res;

            if (yA1 - yA0 > res / 2)
                yA1 -= res;
            else if (yA0 - yA1 > res / 2)
                yA1 += res;

            drawLine(bufferA, xA0, yA0, xA1, yA1);

            // sutf B
            int xB0 = (int)(intersectionPoints[i].uB * res);
            xB0 = (xB0 % res + res) % res;

            int yB0 = (int)(intersectionPoints[i].vB * res);
            yB0 = (yB0 % res + res) % res;

            int xB1 = (int)(intersectionPoints[i+1].uB * res);
            xB1 = (xB1 % res + res) % res;

            int yB1 = (int)(intersectionPoints[i+1].vB * res);
            yB1 = (yB1 % res + res) % res;

            if (xB1 - xB0 > res / 2)
                xB1 -= res;
            else if (xB0 - xB1 > res / 2)
                xB1 += res;

            if (yB1 - yB0 > res / 2)
                yB1 -= res;
            else if (yB0 - yB1 > res / 2)
                yB1 += res;

            drawLine(bufferB, xB0, yB0, xB1, yB1);
        }
    }


    auto sealGap = [&](std::vector<unsigned char>& buf, float u, float v, bool wU, bool wV) {
        int x = (int)(u * res);
        x = (x % res + res) % res;

        int y = (int)(v * res);
        y = (y % res + res) % res;

        int threshold = (int)(res * 0.05f); // Tolerancja 5%

        if (!wU)
        {
            if (x <= threshold) drawLine(buf, x, y, 0, y);
            if (x >= res - threshold) drawLine(buf, x, y, res - 1, y);
        }
        if (!wV)
        {
            if (y <= threshold) drawLine(buf, x, y, x, 0);
            if (y >= res - threshold) drawLine(buf, x, y, x, res - 1);
        }
    };

    if (!intersectionPoints.empty())
    {
        sealGap(bufferA, intersectionPoints.front().uA, intersectionPoints.front().vA, wrapUA, wrapVA);
        sealGap(bufferA, intersectionPoints.back().uA, intersectionPoints.back().vA, wrapUA, wrapVA);

        sealGap(bufferB, intersectionPoints.front().uB, intersectionPoints.front().vB, wrapUB, wrapVB);
        sealGap(bufferB, intersectionPoints.back().uB, intersectionPoints.back().vB, wrapUB, wrapVB);
    }


    auto drawBorder = [&](std::vector<unsigned char>& buf, bool wrapU, bool wrapV) {
        if (!wrapU)
        {
            for(int y = 0; y < res; ++y)
            {
                int i1 = (y * res + 0) * 4;       buf[i1] = buf[i1+1] = buf[i1+2] = 0; // Lewa
                int i2 = (y * res + (res-1)) * 4; buf[i2] = buf[i2+1] = buf[i2+2] = 0; // Prawa
            }
        }
        if (!wrapV)
        {
            for(int x = 0; x < res; ++x)
            {
                int i1 = (0 * res + x) * 4;       buf[i1] = buf[i1+1] = buf[i1+2] = 0; // Góra
                int i2 = ((res-1) * res + x) * 4; buf[i2] = buf[i2+1] = buf[i2+2] = 0; // Dół
            }
        }
    };

    drawBorder(bufferA, wrapUA, wrapVA);
    drawBorder(bufferB, wrapUB, wrapVB);


    auto floodFill = [&](std::vector<unsigned char>& buf, bool wrapU, bool wrapV) {
        int startX = -1, startY = -1;
        // Szukamy pierwszego białego piksela
        for (int y = 0; y < res; ++y)
        {
            for (int x = 0; x < res; ++x)
            {
                if (buf[(y * res + x) * 4] == 255)
                {
                    startX = x;
                    startY = y;
                    break;
                }
            }
            if (startX != -1)
                break;
        }
        if (startX == -1)
            return;

        std::vector<std::pair<int, int>> queue;
        queue.reserve(res * res); 
        queue.push_back({startX, startY});
        buf[(startY * res + startX) * 4] = 128; // Odwiedzony (Szary)

        int head = 0;
        while(head < queue.size())
        {
            int cx = queue[head].first;
            int cy = queue[head].second;
            head++;

            int dx[] = {1, -1, 0, 0};
            int dy[] = {0, 0, 1, -1};
            for (int i = 0; i < 4; ++i)
            {
                int nx = cx + dx[i];
                int ny = cy + dy[i];

                if (wrapU)
                    nx = (nx + res) % res;
                if (wrapV)
                    ny = (ny + res) % res;

                if (nx >= 0 && nx < res && ny >= 0 && ny < res)
                {
                    int idx = (ny * res + nx) * 4;
                    if (buf[idx] == 255)
                    {
                        buf[idx] = 128;
                        queue.push_back({nx, ny});
                    }
                }
            }
        }

        // black white final
        for (int i = 0; i < res * res; ++i)
        {
            if (buf[i * 4] == 128)
            {
                buf[i * 4] = buf[i * 4 + 1] = buf[i * 4 + 2] = 0;
            }
            else
            {
                buf[i * 4] = buf[i * 4 + 1] = buf[i * 4 + 2] = 255;
            }
        }
    };

    floodFill(bufferA, wrapUA, wrapVA);
    floodFill(bufferB, wrapUB, wrapVB);


    auto cleanupBorder = [&](std::vector<unsigned char>& buf, bool wrapU, bool wrapV) {
        if (!wrapU)
        {
            for (int y = 1; y < res - 1; ++y)
            {
                int idxL0 = (y * res + 0) * 4;
                int idxL1 = (y * res + 1) * 4;
                buf[idxL0] = buf[idxL1];
                buf[idxL0+1] = buf[idxL1+1];
                buf[idxL0+2] = buf[idxL1+2];

                int idxR0 = (y * res + (res - 1)) * 4;
                int idxR1 = (y * res + (res - 2)) * 4;
                buf[idxR0] = buf[idxR1];
                buf[idxR0+1] = buf[idxR1+1];
                buf[idxR0+2] = buf[idxR1+2];
            }
        }
        if (!wrapV)
        {
            for (int x = 1; x < res - 1; ++x)
            {
                int idxT0 = (0 * res + x) * 4;
                int idxT1 = (1 * res + x) * 4;
                buf[idxT0] = buf[idxT1];
                buf[idxT0+1] = buf[idxT1+1];
                buf[idxT0+2] = buf[idxT1+2];

                int idxB0 = ((res - 1) * res + x) * 4;
                int idxB1 = ((res - 2) * res + x) * 4;
                buf[idxB0] = buf[idxB1];
                buf[idxB0+1] = buf[idxB1+1];
                buf[idxB0+2] = buf[idxB1+2];
            }
        }

        auto copyCorner = [&](int dx, int dy, int sx, int sy) {
            int d_idx = (dy * res + dx) * 4;
            int s_idx = (sy * res + sx) * 4;
            buf[d_idx] = buf[s_idx];
            buf[d_idx+1] = buf[s_idx+1];
            buf[d_idx+2] = buf[s_idx+2];
        };
        if (!wrapU || !wrapV)
        {
            copyCorner(0,       0,       1,       1);
            copyCorner(res-1,   0,       res-2,   1);
            copyCorner(0,       res-1,   1,       res-2);
            copyCorner(res-1,   res-1,   res-2,   res-2);
        }
    };

    cleanupBorder(bufferA, wrapUA, wrapVA);
    cleanupBorder(bufferB, wrapUB, wrapVB);


    auto uploadTexture = [](GLuint& tex, const std::vector<unsigned char>& buf, int r) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, r, r, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    };

    uploadTexture(textureA, bufferA, res);
    uploadTexture(textureB, bufferB, res);
}