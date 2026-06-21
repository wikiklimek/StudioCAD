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

    virtual bool isWrappedU() const { return isCylinder; } // Walec zapętla się po U
    virtual bool isWrappedV() const { return false; }      // V idzie od dołu do góry (nie zapętla się)

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
    // ---- BERNSTEIN (C0) ----
    float B0(float t) { float inv = 1.0f - t; return inv * inv * inv; }
    float B1(float t) { float inv = 1.0f - t; return 3.0f * t * inv * inv; }
    float B2(float t) { float inv = 1.0f - t; return 3.0f * t * t * inv; }
    float B3(float t) { return t * t * t; }

    // Pochodne Bernsteina
    float dB0(float t) { float inv = 1.0f - t; return -3.0f * inv * inv; }
    float dB1(float t) { float inv = 1.0f - t; return 3.0f * inv * inv - 6.0f * t * inv; }
    float dB2(float t) { float inv = 1.0f - t; return 6.0f * t * inv - 3.0f * t * t; }
    float dB3(float t) { return 3.0f * t * t; }

    float Bernstein(int i, float t) {
        if(i==0) return B0(t); if(i==1) return B1(t); if(i==2) return B2(t); return B3(t);
    }
    float dBernstein(int i, float t) {
        if(i==0) return dB0(t); if(i==1) return dB1(t); if(i==2) return dB2(t); return dB3(t);
    }

    // ---- B-SPLINE (C2) ----
    float N0(float t) { float inv = 1.0f - t; return (inv * inv * inv) / 6.0f; }
    float N1(float t) { return (3.0f*t*t*t - 6.0f*t*t + 4.0f) / 6.0f; }
    float N2(float t) { return (-3.0f*t*t*t + 3.0f*t*t + 3.0f*t + 1.0f) / 6.0f; }
    float N3(float t) { return (t * t * t) / 6.0f; }

    // Pochodne B-Spline
    float dN0(float t) { float inv = 1.0f - t; return -0.5f * inv * inv; }
    float dN1(float t) { return 1.5f*t*t - 2.0f*t; }
    float dN2(float t) { return -1.5f*t*t + t + 0.5f; }
    float dN3(float t) { return 0.5f * t * t; }

    float BSpline(int i, float t) {
        if(i==0) return N0(t); if(i==1) return N1(t); if(i==2) return N2(t); return N3(t);
    }
    float dBSpline(int i, float t) {
        if(i==0) return dN0(t); if(i==1) return dN1(t); if(i==2) return dN2(t); return dN3(t);
    }
}


