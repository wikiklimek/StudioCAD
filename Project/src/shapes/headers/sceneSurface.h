#pragma once
#include "sceneObject.h"
#include "scenePoint.h"
#include "previewContext.h"
#include "scenePolygon.h"
#include <vector>
#include <memory>
#include <algorithm>

class SceneSurface : public SceneObject, public ScenePolygon {
public:
    int sizeU = 0, sizeV = 0;
    int samplesU = 4, samplesV = 4;

    bool isCylinder = false;
    bool showPolygon = true;

    std::vector<std::weak_ptr<ScenePoint>> points;
    std::vector<Vect3> prevPositions;

    SceneSurface(std::string n, Transformations spawnTransform, ObjectType type);
    virtual ~SceneSurface();

    void Init() override = 0;

    virtual void DrawSurface(Shader& shader, const PreviewContext& ctx) = 0;
    void DrawPolygon(Shader& lineShader, const PreviewContext& ctx) override;

    void Draw(Shader& shader) override {}
    void Draw(Shader& shader, Mat4 parentMatrix) override {}

    virtual Vect3 EvaluatePos(float u, float v) const = 0;
    virtual Vect3 EvaluateDu(float u, float v) const = 0;
    virtual Vect3 EvaluateDv(float u, float v) const = 0;

    virtual bool isWrappedU() const { return isCylinder; } 
    virtual bool isWrappedV() const { return false; }      

protected:
    unsigned int VAO_surface = 0, VBO = 0, EBO_surface = 0;
    unsigned int VAO_poly = 0, EBO_poly = 0;

    std::vector<unsigned int> patchIndices;
    std::vector<unsigned int> polyIndices;

    void RenderSurfaceInternal(Shader& shader, const PreviewContext& ctx);

    void InitBuffers();
    void InitPolygonAndUpload();
};


class SceneSurfaceC0 : public SceneSurface {
public:
    SceneSurfaceC0(std::string n, Transformations spawnTransform)
            : SceneSurface(std::move(n), spawnTransform, ObjectType::BezierSurfaceC0) {}

    void Init() override;
    void DrawSurface(Shader& shader, const PreviewContext& ctx) override {
        shader.use();
        glUniform1i(glGetUniformLocation(shader.ID, "patchesU"), (sizeU - 1) / 3);
        glUniform1i(glGetUniformLocation(shader.ID, "patchesV"), (sizeV - 1) / 3);
        RenderSurfaceInternal(shader, ctx);
    }

    Vect3 EvaluatePos(float u, float v) const override;
    Vect3 EvaluateDu(float u, float v) const override;
    Vect3 EvaluateDv(float u, float v) const override;
};

class SceneSurfaceC2 : public SceneSurface {
public:
    SceneSurfaceC2(std::string n, Transformations spawnTransform)
            : SceneSurface(std::move(n), spawnTransform, ObjectType::BezierSurfaceC2) {}

    void Init() override;
    void DrawSurface(Shader& shader, const PreviewContext& ctx) override {
        shader.use();
        glUniform1i(glGetUniformLocation(shader.ID, "patchesU"), isCylinder ? (sizeU - 1) : (sizeU - 3));
        glUniform1i(glGetUniformLocation(shader.ID, "patchesV"), sizeV - 3);
        RenderSurfaceInternal(shader, ctx);
    }

    Vect3 EvaluatePos(float u, float v) const override;
    Vect3 EvaluateDu(float u, float v) const override;
    Vect3 EvaluateDv(float u, float v) const override;
};



namespace {
    constexpr float B0(float t) { float inv = 1.0f - t; return inv * inv * inv; }
    constexpr float B1(float t) { float inv = 1.0f - t; return 3.0f * t * inv * inv; }
    constexpr float B2(float t) { float inv = 1.0f - t; return 3.0f * t * t * inv; }
    constexpr float B3(float t) { return t * t * t; }

    constexpr float dB0(float t) { float inv = 1.0f - t; return -3.0f * inv * inv; }
    constexpr float dB1(float t) { float inv = 1.0f - t; return 3.0f * inv * inv - 6.0f * t * inv; }
    constexpr float dB2(float t) { float inv = 1.0f - t; return 6.0f * t * inv - 3.0f * t * t; }
    constexpr float dB3(float t) { return 3.0f * t * t; }

    
    constexpr float (*B[])(float) = { B0, B1, B2, B3 };
    constexpr float (*dB[])(float) = { dB0, dB1, dB2, dB3 };

    constexpr float Bernstein(int i, float t) 
    {
        return B[i](t);
    }

    constexpr float dBernstein(int i, float t) 
    {
        return dB[i](t);
    }

    
    constexpr float N0(float t) { float inv = 1.0f - t; return (inv * inv * inv) / 6.0f; }
    constexpr float N1(float t) { return (3.0f * t * t * t - 6.0f * t * t + 4.0f) / 6.0f; }
    constexpr float N2(float t) { return (-3.0f * t * t * t + 3.0f * t * t + 3.0f * t + 1.0f) / 6.0f; }
    constexpr float N3(float t) { return (t * t * t) / 6.0f; }

    constexpr float dN0(float t) { float inv = 1.0f - t; return -0.5f * inv * inv; }
    constexpr float dN1(float t) { return 1.5f * t * t - 2.0f * t; }
    constexpr float dN2(float t) { return -1.5f * t * t + t + 0.5f; }
    constexpr float dN3(float t) { return 0.5f * t * t; }

    constexpr float (*N[])(float) = { N0, N1, N2, N3 };
    constexpr float (*dN[])(float) = { dN0, dN1, dN2, dN3 };

    constexpr float BSpline(int i, float t) 
    {
        return N[i](t);
    }

    constexpr float dBSpline(int i, float t) 
    {
        return dN[i](t);
    }
}


