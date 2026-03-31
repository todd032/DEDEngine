#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "core/mesh_types.hpp"
#include "engine/types.hpp"

namespace cotrx::viewer
{

struct OrbitCameraState
{
    float yawRadians = 0.75f;
    float pitchRadians = 0.45f;
    float distance = 7.5f;
    std::array<float, 3> target{0.0f, 0.0f, 0.0f};

    void Orbit(float deltaYawRadians, float deltaPitchRadians);
    void Pan(float deltaX, float deltaY);
    void Zoom(float amount);
};

struct CutPlaneState
{
    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 3> rotationEulerRadians{0.0f, 0.0f, 0.0f};

    void MoveBy(const std::array<float, 3>& delta);
    void RotateBy(const std::array<float, 3>& deltaRadians);
};

enum class SurfaceColorMode : std::uint8_t
{
    Default = 0,
    SurfaceTypeRule
};

struct ViewerOverlayState
{
    bool showWireframe = false;
    bool showNormals = false;
    bool showFragmentId = true;
    bool showTriangleStats = true;
};

struct FragmentStats
{
    std::uint32_t fragmentId = 0;
    std::size_t triangleCount = 0;
    float totalArea = 0.0f;
};

struct ViewerState
{
    OrbitCameraState camera;
    CutPlaneState cutPlane;
    ViewerOverlayState overlay;
    SurfaceColorMode colorMode = SurfaceColorMode::Default;

    std::vector<core::Fragment> fragments;
    std::size_t selectedFragmentIndex = 0;
    std::vector<FragmentStats> stats;

    std::size_t lastSceneMeshCount = 0;
    std::size_t lastSceneTriangleCount = 0;
    std::size_t overlayDrawCallCount = 0;
    std::size_t overlayPrimitiveCount = 0;

    bool running = false;
    bool requestedExportObj = false;
    bool requestedExportJson = false;
};

[[nodiscard]] std::vector<MeshData> BuildSceneMeshesFromFragments(
    const std::vector<core::Fragment>& fragments,
    SurfaceColorMode colorMode);

class ViewerApp
{
public:
    ViewerApp();

    bool Initialize();
    void Shutdown();
    void Run();

    [[nodiscard]] ViewerState& State() noexcept;
    [[nodiscard]] const ViewerState& State() const noexcept;

    bool ResliceSelectedFragment();
    bool ExportObj(const std::string& filePath) const;
    bool ExportJson(const std::string& filePath) const;

private:
    void UpdateStats();

    ViewerState state_{};
    bool initialized_ = false;
};

void main_module_anchor();

} // namespace cotrx::viewer
