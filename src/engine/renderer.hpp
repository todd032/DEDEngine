#pragma once

#include <vector>

#include "engine/gl_api.hpp"
#include "game/simulation.hpp"

namespace cotrx
{
class Renderer
{
public:
    Renderer() = default;
    ~Renderer();

    bool Initialize(bool glesContext);
    void Shutdown();
    void RebuildScene(const SimulationState& state);
    void Render(const SimulationState& state, const Color& clearColor, int uiWidth, int uiHeight, int drawableWidth, int drawableHeight);
    void Render(
        const SimulationState& state,
        const UiOverlayState& uiState,
        const Color& clearColor,
        int uiWidth,
        int uiHeight,
        int drawableWidth,
        int drawableHeight);

private:
    struct GpuMesh
    {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLsizei indexCount = 0;
    };

    struct UiVertex
    {
        float x = 0.0f;
        float y = 0.0f;
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    bool CreatePrograms(bool glesContext);
    bool UploadMesh(const MeshData& mesh, GpuMesh& gpuMesh);
    void DestroyMeshes();
    void BuildUiVertices(const SimulationState& state, int width, int height, std::vector<UiVertex>& vertices) const;
    void BuildUiVertices(const UiOverlayState& uiState, int width, int height, std::vector<UiVertex>& vertices) const;
    void AppendRect(std::vector<UiVertex>& vertices, float x, float y, float width, float height, const Color& color) const;
    void AppendFrame(std::vector<UiVertex>& vertices, float x, float y, float width, float height, float thickness, const Color& color) const;
    void AppendText(std::vector<UiVertex>& vertices, float x, float y, float scale, const std::string& text, const Color& color) const;

    GLuint sceneProgram_ = 0;
    GLuint uiProgram_ = 0;
    GLint sceneModelLocation_ = -1;
    GLint sceneViewProjectionLocation_ = -1;
    GLint sceneTintLocation_ = -1;
    GLint sceneLightLocation_ = -1;
    GLint uiProjectionLocation_ = -1;
    GLuint uiVao_ = 0;
    GLuint uiVbo_ = 0;
    bool glesContext_ = false;
    std::vector<GpuMesh> sceneMeshes_;
};
} // namespace cotrx
