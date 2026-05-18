


#include "shader.h"
#include "previewContext.h"

class ScenePolygon{
public:
    virtual void DrawPolygon(Shader& lineShader, const PreviewContext& ctx) = 0;
};
