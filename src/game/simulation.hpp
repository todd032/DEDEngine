#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "engine/types.hpp"

namespace cotrx
{
enum class RoofShapeKind : std::uint8_t
{
    Flat = 0,
    Gable,
    Dome,
    Pyramid,
    Wave,
    Shed,
};

struct SurfaceSample
{
    Vec3 point;
    Vec3 normal;
};

struct RoofDefinition
{
    RoofShapeKind shapeKind = RoofShapeKind::Flat;
    float baseY = 3.4f;
    float width = 10.0f;
    float depth = 10.0f;
    float height = 0.8f;
    float ridgeGap = 0.0f;
    float primaryVariation = 0.0f;
    float secondaryVariation = 0.0f;
    float phase = 0.0f;

    [[nodiscard]] std::string DisplayName() const;
    [[nodiscard]] Vec3 CenterPoint() const;
    [[nodiscard]] SurfaceSample Sample(float x, float z) const;
    [[nodiscard]] float SampleHeight(float x, float z) const;
    [[nodiscard]] Vec3 SampleNormal(float x, float z) const;
    [[nodiscard]] MeshData BuildMesh(int gridDivisions, const Color& roofColor) const;

    static RoofDefinition CreateFlat(float baseY, float width, float depth, float height);
    static RoofDefinition CreateRandom(std::mt19937& generator, float baseY, float width, float depth, float maxHeight);
};

struct CookieSpec
{
    float minWidth = 1.0f;
    float maxWidth = 2.0f;
    float minDepth = 1.0f;
    float maxDepth = 2.0f;
    float minHeight = 0.2f;
    float maxHeight = 0.3f;
};

struct CookiePlacement
{
    Vec3 center;
    Vec3 xAxis;
    Vec3 yAxis;
    Vec3 zAxis;
    float width = 1.0f;
    float depth = 1.0f;
    float height = 0.2f;
    Color color;
};

struct SimulationState
{
    RoofDefinition roof;
    CookieSpec cookieSpec;
    std::vector<CookiePlacement> cookies;
    OrbitCamera camera;
    UiAction pendingAction = UiAction::None;
    std::vector<MeshData> meshes;
    std::vector<SceneObject> sceneObjects;
    std::vector<ButtonState> buttons;
    std::vector<std::string> hudLines;
    int cookieCount = 0;
    std::uint32_t seed = 1337u;
};

class Simulation
{
public:
    explicit Simulation(std::uint32_t seed = 1337u);

    void SetViewportSize(int width, int height);
    void SetRoof(const RoofDefinition& roof);
    void RandomizeRoof();
    void RedistributeCookies();
    void QueueAction(UiAction action);
    void OrbitFromPixels(float deltaX, float deltaY);
    void ZoomFromInput(float zoomDelta);
    void UpdateHover(float x, float y);
    void SetPressedAction(UiAction action);

    [[nodiscard]] UiAction HitTestButton(float x, float y) const;
    [[nodiscard]] const SimulationState& State() const noexcept;
    [[nodiscard]] std::uint64_t SceneRevision() const noexcept;

private:
    void ProcessPendingAction();
    void RebuildScene();
    void RefreshButtons();
    void RefreshHud();
    std::vector<CookiePlacement> CreateCookiePlacements(const RoofDefinition& roof);
    std::vector<CookiePlacement> CreateSurfaceCookiePlacements(const RoofDefinition& roof);
    std::vector<CookiePlacement> CreateGableCookiePlacements(const RoofDefinition& roof);

    SimulationState state_{};
    std::mt19937 random_;
    std::uint64_t sceneRevision_ = 0;
    int viewportWidth_ = 1360;
    int viewportHeight_ = 820;
};
} // namespace cotrx
