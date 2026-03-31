#include "viewer/main.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "viewer/debug_draw.hpp"
#include "viewer/imgui_panels.hpp"
#include "core/procedural_cube_generator.hpp"

#if __has_include(<GLFW/glfw3.h>)
#define COTRX_VIEWER_HAS_GLFW 1
#include <GLFW/glfw3.h>
#else
#define COTRX_VIEWER_HAS_GLFW 0
struct GLFWwindow;
#endif

#if __has_include(<glad/glad.h>)
#define COTRX_VIEWER_HAS_GLAD 1
#include <glad/glad.h>
#else
#define COTRX_VIEWER_HAS_GLAD 0
#endif

#if __has_include(<glm/glm.hpp>)
#define COTRX_VIEWER_HAS_GLM 1
#include <glm/glm.hpp>
#else
#define COTRX_VIEWER_HAS_GLM 0
#endif

namespace cotrx::viewer
{
namespace
{
float Clamp(float value, float low, float high)
{
    return std::max(low, std::min(value, high));
}

float TriangleArea(const Vec3& a, const Vec3& b, const Vec3& c)
{
    const auto ab = b - a;
    const auto ac = c - a;
    return 0.5f * Length(Cross(ab, ac));
}

void ResliceInPlace(core::Fragment& fragment)
{
    // Temporary placeholder for per-fragment reslicing pipeline.
    // Keep deterministic and visible so QA can verify that only one selected
    // fragment was touched.
    std::reverse(fragment.indices.begin(), fragment.indices.end());
}

Color ColorForSurfaceType(const core::SurfaceType surfaceType, const SurfaceColorMode colorMode)
{
    if (colorMode != SurfaceColorMode::SurfaceTypeRule)
    {
        return {0.82f, 0.75f, 0.70f, 1.0f};
    }

    switch (surfaceType)
    {
    case core::SurfaceType::OuterSurface:
    case core::SurfaceType::CutSurface:
        return {0.89f, 0.72f, 0.62f, 1.0f}; // skin tone
    case core::SurfaceType::InnerSurface:
    case core::SurfaceType::CutCap:
        return {0.74f, 0.27f, 0.25f, 1.0f}; // meat tone
    }

    return {1.0f, 1.0f, 1.0f, 1.0f};
}

void RenderSceneMeshes(ViewerState& state, const std::vector<MeshData>& sceneMeshes)
{
    state.lastSceneMeshCount = sceneMeshes.size();
    state.lastSceneTriangleCount = 0;
    for (const auto& mesh : sceneMeshes)
    {
        state.lastSceneTriangleCount += mesh.indices.size() / 3;
    }
}

} // namespace

std::vector<MeshData> BuildSceneMeshesFromFragments(
    const std::vector<core::Fragment>& fragments,
    const SurfaceColorMode colorMode)
{
    std::vector<MeshData> meshes;
    meshes.reserve(fragments.size());

    for (const auto& fragment : fragments)
    {
        MeshData mesh{};
        mesh.indices = fragment.indices;
        mesh.vertices.reserve(fragment.vertices.size());

        for (const auto& position : fragment.vertices)
        {
            mesh.vertices.push_back(Vertex{position, Vec3{0.0f, 0.0f, 0.0f}, ColorForSurfaceType(core::SurfaceType::OuterSurface, colorMode)});
            ExpandBounds(mesh.bounds, position);
        }

        for (std::size_t tri = 0; tri + 2 < fragment.indices.size(); tri += 3)
        {
            const auto i0 = static_cast<std::size_t>(fragment.indices[tri + 0]);
            const auto i1 = static_cast<std::size_t>(fragment.indices[tri + 1]);
            const auto i2 = static_cast<std::size_t>(fragment.indices[tri + 2]);
            if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size())
            {
                continue;
            }

            const core::SurfaceType surfaceType =
                tri / 3 < fragment.triangleSurfaceTags.size() ? fragment.triangleSurfaceTags[tri / 3] : core::SurfaceType::OuterSurface;
            const Color vertexColor = ColorForSurfaceType(surfaceType, colorMode);
            mesh.vertices[i0].color = vertexColor;
            mesh.vertices[i1].color = vertexColor;
            mesh.vertices[i2].color = vertexColor;

            const Vec3 normal = Normalize(Cross(
                mesh.vertices[i1].position - mesh.vertices[i0].position,
                mesh.vertices[i2].position - mesh.vertices[i0].position));
            mesh.vertices[i0].normal = mesh.vertices[i0].normal + normal;
            mesh.vertices[i1].normal = mesh.vertices[i1].normal + normal;
            mesh.vertices[i2].normal = mesh.vertices[i2].normal + normal;
        }

        for (auto& vertex : mesh.vertices)
        {
            vertex.normal = Normalize(vertex.normal);
        }

        meshes.push_back(std::move(mesh));
    }

    return meshes;
}

void OrbitCameraState::Orbit(const float deltaYawRadians, const float deltaPitchRadians)
{
    yawRadians += deltaYawRadians;
    pitchRadians = Clamp(pitchRadians + deltaPitchRadians, -1.45f, 1.45f);
}

void OrbitCameraState::Pan(const float deltaX, const float deltaY)
{
    target[0] += deltaX;
    target[1] += deltaY;
}

void OrbitCameraState::Zoom(const float amount)
{
    distance = Clamp(distance + amount, 0.5f, 500.0f);
}

void CutPlaneState::MoveBy(const std::array<float, 3>& delta)
{
    for (std::size_t i = 0; i < position.size(); ++i)
    {
        position[i] += delta[i];
    }
}

void CutPlaneState::RotateBy(const std::array<float, 3>& deltaRadians)
{
    for (std::size_t i = 0; i < rotationEulerRadians.size(); ++i)
    {
        rotationEulerRadians[i] += deltaRadians[i];
    }
}

ViewerApp::ViewerApp()
{
    state_.selectedFragmentIndex = 0;
}

bool ViewerApp::Initialize()
{
#if COTRX_VIEWER_HAS_GLFW
    if (glfwInit() == GLFW_FALSE)
    {
        return false;
    }

#if COTRX_VIEWER_HAS_GLAD
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
#endif

#if COTRX_VIEWER_HAS_GLM
    [[maybe_unused]] const auto sanity = glm::vec3{0.0f, 0.0f, 0.0f};
#endif

    initialized_ = true;
    state_.running = true;
    if (state_.fragments.empty())
    {
        const auto seedPair = core::GenerateProceduralCubeShellPair(core::ProceduralCubeGeneratorConfig{});
        state_.fragments.push_back(seedPair.outerShell);
        state_.fragments.push_back(seedPair.innerShell);
    }
    UpdateStats();
    return true;
}

void ViewerApp::Shutdown()
{
#if COTRX_VIEWER_HAS_GLFW
    glfwTerminate();
#endif
    state_.running = false;
    initialized_ = false;
}

void ViewerApp::Run()
{
    if (!initialized_)
    {
        return;
    }

    while (state_.running)
    {
        const auto sceneMeshes = BuildSceneMeshesFromFragments(state_.fragments, state_.colorMode);
        RenderSceneMeshes(state_, sceneMeshes);

        BuildManipulatorPanel(state_);
        BuildRenderModePanel(state_);
        BuildExportPanel(state_);
        BuildSelectionPanel(state_);

        if (state_.requestedExportObj)
        {
            ExportObj("viewer_state.obj");
            state_.requestedExportObj = false;
        }

        if (state_.requestedExportJson)
        {
            ExportJson("viewer_state.json");
            state_.requestedExportJson = false;
        }

        // Render debug overlays (wireframe/normals/id/stats).
        DrawViewerOverlays(state_);

        // Headless safety break for environments without event pumps.
        state_.running = false;
    }
}

ViewerState& ViewerApp::State() noexcept
{
    return state_;
}

const ViewerState& ViewerApp::State() const noexcept
{
    return state_;
}

bool ViewerApp::ResliceSelectedFragment()
{
    if (state_.selectedFragmentIndex >= state_.fragments.size())
    {
        return false;
    }

    ResliceInPlace(state_.fragments[state_.selectedFragmentIndex]);
    UpdateStats();
    return true;
}

bool ViewerApp::ExportObj(const std::string& filePath) const
{
    std::ofstream output(filePath, std::ios::trunc);
    if (!output.is_open())
    {
        return false;
    }

    std::uint32_t vertexBase = 1;
    for (const auto& fragment : state_.fragments)
    {
        output << "o fragment_" << fragment.id << "\n";
        for (const auto& vertex : fragment.vertices)
        {
            output << "v " << vertex.x << ' ' << vertex.y << ' ' << vertex.z << "\n";
        }

        for (std::size_t i = 0; i + 2 < fragment.indices.size(); i += 3)
        {
            const auto a = fragment.indices[i + 0] + vertexBase;
            const auto b = fragment.indices[i + 1] + vertexBase;
            const auto c = fragment.indices[i + 2] + vertexBase;
            output << "f " << a << ' ' << b << ' ' << c << "\n";
        }

        vertexBase += static_cast<std::uint32_t>(fragment.vertices.size());
    }

    return true;
}

bool ViewerApp::ExportJson(const std::string& filePath) const
{
    std::ofstream output(filePath, std::ios::trunc);
    if (!output.is_open())
    {
        return false;
    }

    output << "{\n";
    output << "  \"selectedFragmentIndex\": " << state_.selectedFragmentIndex << ",\n";
    output << "  \"surfaceColorMode\": \""
           << (state_.colorMode == SurfaceColorMode::SurfaceTypeRule ? "surface_type_rule" : "default")
           << "\",\n";
    output << "  \"fragments\": [\n";

    for (std::size_t i = 0; i < state_.fragments.size(); ++i)
    {
        const auto& fragment = state_.fragments[i];
        output << "    {\n";
        output << "      \"id\": " << fragment.id << ",\n";
        output << "      \"vertexCount\": " << fragment.vertices.size() << ",\n";
        output << "      \"triangleCount\": " << (fragment.indices.size() / 3) << "\n";
        output << "    }";
        if (i + 1 < state_.fragments.size())
        {
            output << ',';
        }
        output << "\n";
    }

    output << "  ]\n";
    output << "}\n";

    return true;
}

void ViewerApp::UpdateStats()
{
    state_.stats.clear();
    state_.stats.reserve(state_.fragments.size());

    for (const auto& fragment : state_.fragments)
    {
        FragmentStats stats{};
        stats.fragmentId = fragment.id;
        stats.triangleCount = fragment.indices.size() / 3;

        for (std::size_t i = 0; i + 2 < fragment.indices.size(); i += 3)
        {
            const auto ia = static_cast<std::size_t>(fragment.indices[i + 0]);
            const auto ib = static_cast<std::size_t>(fragment.indices[i + 1]);
            const auto ic = static_cast<std::size_t>(fragment.indices[i + 2]);
            if (ia < fragment.vertices.size() && ib < fragment.vertices.size() && ic < fragment.vertices.size())
            {
                stats.totalArea += TriangleArea(fragment.vertices[ia], fragment.vertices[ib], fragment.vertices[ic]);
            }
        }

        state_.stats.push_back(stats);
    }
}

void main_module_anchor()
{
}

} // namespace cotrx::viewer
