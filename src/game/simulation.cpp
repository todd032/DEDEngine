#include "game/simulation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <random>
#include <sstream>

#include "engine/geometry.hpp"

namespace cotrx
{
namespace
{
constexpr float RoofWidth = 10.0f;
constexpr float RoofDepth = 10.0f;
constexpr float MaxRoofHeight = 4.0f;
constexpr float BuildingBodyHeight = 3.4f;
constexpr float GroundSize = 34.0f;
constexpr float MinOrbitYawSpeed = 0.0105f;
constexpr float MinOrbitPitchSpeed = 0.0085f;
constexpr int SurfaceGridDivisions = 32;

constexpr Color RoofColor{0.9137f, 0.4745f, 0.1451f, 1.0f};
constexpr Color BodyColor{0.7843f, 0.8078f, 0.8314f, 1.0f};
constexpr Color GroundWarm{0.9255f, 0.9020f, 0.8627f, 1.0f};
constexpr Color GroundCool{0.8667f, 0.8941f, 0.9255f, 1.0f};
constexpr Color ShadowColor{0.7765f, 0.7333f, 0.6745f, 1.0f};
constexpr std::array<Color, 6> CookiePalette{{
    {0.2431f, 0.5686f, 0.7882f, 1.0f},
    {0.3922f, 0.7412f, 0.4941f, 1.0f},
    {0.8471f, 0.3529f, 0.4078f, 1.0f},
    {0.6706f, 0.5137f, 0.8941f, 1.0f},
    {0.9529f, 0.7412f, 0.2627f, 1.0f},
    {0.3373f, 0.8039f, 0.7765f, 1.0f},
}};

float NextRange(std::mt19937& generator, float minimum, float maximum)
{
    std::uniform_real_distribution<float> distribution(minimum, maximum);
    return distribution(generator);
}

int NextIndex(std::mt19937& generator, int count)
{
    std::uniform_int_distribution<int> distribution(0, count - 1);
    return distribution(generator);
}

Vec3 ProjectedAxis(const Vec3& normal, const Vec3& preferredAxis)
{
    auto projected = preferredAxis - (normal * Dot(preferredAxis, normal));
    if (LengthSquared(projected) <= 1.0e-6f)
    {
        projected = Vec3{0.0f, 0.0f, 1.0f} - (normal * Dot(Vec3{0.0f, 0.0f, 1.0f}, normal));
    }

    return Normalize(projected);
}

std::vector<float> CreatePartition(float total, float minSegment, float maxSegment, std::mt19937& generator)
{
    if (total <= minSegment)
    {
        return {total};
    }

    const auto minimumCount = static_cast<int>(std::ceil(total / maxSegment));
    const auto maximumCount = std::max(minimumCount, static_cast<int>(std::floor(total / minSegment)));
    const auto preferredMaximum = std::min(maximumCount, minimumCount + 3);
    std::uniform_int_distribution<int> countDistribution(minimumCount, preferredMaximum);
    const auto count = countDistribution(generator);

    std::vector<float> segments;
    segments.reserve(static_cast<std::size_t>(count));
    auto remaining = total;

    for (int index = 0; index < count; ++index)
    {
        const auto remainingCount = count - index - 1;
        const auto minValue = std::max(minSegment, remaining - (static_cast<float>(remainingCount) * maxSegment));
        const auto maxValue = std::min(maxSegment, remaining - (static_cast<float>(remainingCount) * minSegment));
        const auto value = index == (count - 1) ? remaining : Lerp(minValue, maxValue, NextRange(generator, 0.0f, 1.0f));
        segments.push_back(value);
        remaining -= value;
    }

    std::shuffle(segments.begin(), segments.end(), generator);
    return segments;
}

std::vector<float> GetSegmentCenters(const std::vector<float>& segments, float start)
{
    std::vector<float> centers;
    centers.reserve(segments.size());
    auto cursor = start;
    for (const auto segment : segments)
    {
        centers.push_back(cursor + (segment * 0.5f));
        cursor += segment;
    }

    return centers;
}

MeshData CreateGroundMesh()
{
    MeshData ground{};
    constexpr float tileSize = 2.0f;

    for (float x = -GroundSize * 0.5f; x < GroundSize * 0.5f; x += tileSize)
    {
        for (float z = -GroundSize * 0.5f; z < GroundSize * 0.5f; z += tileSize)
        {
            const auto tileIndex = static_cast<int>(((x + (GroundSize * 0.5f)) / tileSize) + ((z + (GroundSize * 0.5f)) / tileSize));
            const auto color = (tileIndex % 2 == 0) ? GroundWarm : GroundCool;
            AppendMesh(
                ground,
                CreateQuad(
                    {x, 0.0f, z + tileSize},
                    {x + tileSize, 0.0f, z + tileSize},
                    {x + tileSize, 0.0f, z},
                    {x, 0.0f, z},
                    color));
        }
    }

    return ground;
}

MeshData CreateShadowMesh()
{
    return CreateQuad(
        {-6.3f, 0.01f, 6.3f},
        {6.3f, 0.01f, 6.3f},
        {6.3f, 0.01f, -6.3f},
        {-6.3f, 0.01f, -6.3f},
        ShadowColor);
}

std::string FormatFixed(float value, int digits)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(digits) << value;
    return stream.str();
}

CookiePlacement CreateCookiePlacement(
    const SurfaceSample& sample,
    const Vec3& tangentX,
    const Vec3& tangentZ,
    float width,
    float depth,
    float height,
    const Color& color)
{
    const auto center = sample.point + (sample.normal * ((height * 0.5f) + 0.02f));
    return {
        center,
        Normalize(tangentX),
        Normalize(sample.normal),
        Normalize(tangentZ),
        width,
        depth,
        height,
        color};
}

MeshData BuildCookieMesh(const std::vector<CookiePlacement>& cookies)
{
    MeshData mesh{};
    for (const auto& cookie : cookies)
    {
        AppendMesh(
            mesh,
            CreateOrientedBox(
                cookie.center,
                cookie.xAxis * (cookie.width * 0.5f),
                cookie.yAxis * (cookie.height * 0.5f),
                cookie.zAxis * (cookie.depth * 0.5f),
                cookie.color));
    }

    return mesh;
}
} // namespace

std::string RoofDefinition::DisplayName() const
{
    switch (shapeKind)
    {
    case RoofShapeKind::Flat:
        return "FLAT";
    case RoofShapeKind::Gable:
        return "GABLE";
    case RoofShapeKind::Dome:
        return "DOME";
    case RoofShapeKind::Pyramid:
        return "PYRAMID";
    case RoofShapeKind::Wave:
        return "WAVE";
    case RoofShapeKind::Shed:
        return "SHED";
    }

    return "UNKNOWN";
}

Vec3 RoofDefinition::CenterPoint() const
{
    return {0.0f, baseY + SampleHeight(0.0f, 0.0f), 0.0f};
}

SurfaceSample RoofDefinition::Sample(float x, float z) const
{
    return {
        {x, baseY + SampleHeight(x, z), z},
        SampleNormal(x, z)};
}

float RoofDefinition::SampleHeight(float x, float z) const
{
    const auto halfWidth = width * 0.5f;
    const auto halfDepth = depth * 0.5f;
    const auto xRatio = std::abs(x) / halfWidth;
    const auto zRatio = std::abs(z) / halfDepth;

    switch (shapeKind)
    {
    case RoofShapeKind::Flat:
        return height;
    case RoofShapeKind::Gable:
        return height * std::max(0.0f, 1.0f - xRatio);
    case RoofShapeKind::Dome:
        return height * std::cos((Pi * x) / (2.0f * halfWidth)) * std::cos((Pi * z) / (2.0f * halfDepth));
    case RoofShapeKind::Pyramid:
        return height * std::max(0.0f, 1.0f - std::max(xRatio, zRatio));
    case RoofShapeKind::Wave:
    {
        const auto frequencyX = primaryVariation * Pi / halfWidth;
        const auto frequencyZ = secondaryVariation * Pi / halfDepth;
        const auto waveX = 0.5f + (0.5f * std::sin((frequencyX * x) + phase));
        const auto waveZ = 0.5f + (0.5f * std::cos((frequencyZ * z) - (phase * 0.6f)));
        const auto waveBlend = 0.35f + (0.35f * waveX) + (0.30f * waveZ);
        const auto xEnvelope = x / halfWidth;
        const auto zEnvelope = z / halfDepth;
        const auto envelope = 1.0f - (0.11f * ((xEnvelope * xEnvelope) + (zEnvelope * zEnvelope)));
        return height * waveBlend * envelope;
    }
    case RoofShapeKind::Shed:
        return height * (0.6f - ((0.8f * x) / width));
    }

    return height;
}

Vec3 RoofDefinition::SampleNormal(float x, float z) const
{
    if (shapeKind == RoofShapeKind::Flat)
    {
        return {0.0f, 1.0f, 0.0f};
    }

    const auto halfWidth = width * 0.5f;
    const auto halfDepth = depth * 0.5f;
    auto dHeightDx = 0.0f;
    auto dHeightDz = 0.0f;

    switch (shapeKind)
    {
    case RoofShapeKind::Gable:
        if (x > 1.0e-4f)
        {
            dHeightDx = -(height / halfWidth);
        }
        else if (x < -1.0e-4f)
        {
            dHeightDx = height / halfWidth;
        }
        break;

    case RoofShapeKind::Dome:
    {
        const auto xAngle = (Pi * x) / (2.0f * halfWidth);
        const auto zAngle = (Pi * z) / (2.0f * halfDepth);
        dHeightDx = -(Pi * height / (2.0f * halfWidth)) * std::sin(xAngle) * std::cos(zAngle);
        dHeightDz = -(Pi * height / (2.0f * halfDepth)) * std::cos(xAngle) * std::sin(zAngle);
        break;
    }

    case RoofShapeKind::Pyramid:
    {
        const auto xRatio = std::abs(x) / halfWidth;
        const auto zRatio = std::abs(z) / halfDepth;
        if (xRatio >= zRatio)
        {
            if (x > 1.0e-4f)
            {
                dHeightDx = -(height / halfWidth);
            }
            else if (x < -1.0e-4f)
            {
                dHeightDx = height / halfWidth;
            }
        }
        else
        {
            if (z > 1.0e-4f)
            {
                dHeightDz = -(height / halfDepth);
            }
            else if (z < -1.0e-4f)
            {
                dHeightDz = height / halfDepth;
            }
        }
        break;
    }

    case RoofShapeKind::Wave:
    {
        const auto frequencyX = primaryVariation * Pi / halfWidth;
        const auto frequencyZ = secondaryVariation * Pi / halfDepth;
        const auto waveXDerivative = 0.5f * std::cos((frequencyX * x) + phase) * frequencyX;
        const auto waveZDerivative = -0.5f * std::sin((frequencyZ * z) - (phase * 0.6f)) * frequencyZ;
        const auto waveX = 0.5f + (0.5f * std::sin((frequencyX * x) + phase));
        const auto waveZ = 0.5f + (0.5f * std::cos((frequencyZ * z) - (phase * 0.6f)));
        const auto waveBlend = 0.35f + (0.35f * waveX) + (0.30f * waveZ);
        const auto xEnvelope = x / halfWidth;
        const auto zEnvelope = z / halfDepth;
        const auto envelope = 1.0f - (0.11f * ((xEnvelope * xEnvelope) + (zEnvelope * zEnvelope)));
        const auto blendDx = 0.35f * waveXDerivative;
        const auto blendDz = 0.30f * waveZDerivative;
        const auto envelopeDx = -0.22f * x / (halfWidth * halfWidth);
        const auto envelopeDz = -0.22f * z / (halfDepth * halfDepth);
        dHeightDx = height * ((blendDx * envelope) + (waveBlend * envelopeDx));
        dHeightDz = height * ((blendDz * envelope) + (waveBlend * envelopeDz));
        break;
    }

    case RoofShapeKind::Shed:
        dHeightDx = -(0.8f * height / width);
        break;

    case RoofShapeKind::Flat:
        break;
    }

    return Normalize({-dHeightDx, 1.0f, -dHeightDz});
}

MeshData RoofDefinition::BuildMesh(int gridDivisions, const Color& roofColor) const
{
    if (shapeKind == RoofShapeKind::Flat)
    {
        return CreateBox({0.0f, baseY + (height * 0.5f), 0.0f}, width, height, depth, roofColor);
    }

    return CreateSurfaceSolid(
        gridDivisions,
        gridDivisions,
        [this](float u, float v)
        {
            const auto x = (u - 0.5f) * width;
            const auto z = (v - 0.5f) * depth;
            return Sample(x, z).point;
        },
        [this](float u, float v)
        {
            return Vec3{(u - 0.5f) * width, baseY, (v - 0.5f) * depth};
        },
        roofColor);
}

RoofDefinition RoofDefinition::CreateFlat(float inBaseY, float inWidth, float inDepth, float inHeight)
{
    RoofDefinition roof{};
    roof.shapeKind = RoofShapeKind::Flat;
    roof.baseY = inBaseY;
    roof.width = inWidth;
    roof.depth = inDepth;
    roof.height = inHeight;
    return roof;
}

RoofDefinition RoofDefinition::CreateRandom(std::mt19937& generator, float inBaseY, float inWidth, float inDepth, float maxHeight)
{
    RoofDefinition roof{};
    roof.baseY = inBaseY;
    roof.width = inWidth;
    roof.depth = inDepth;

    switch (NextIndex(generator, 6))
    {
    case 0:
        roof.shapeKind = RoofShapeKind::Flat;
        roof.height = NextRange(generator, 0.65f, 1.2f);
        break;
    case 1:
        roof.shapeKind = RoofShapeKind::Gable;
        roof.height = NextRange(generator, 2.2f, maxHeight);
        roof.ridgeGap = NextRange(generator, 0.7f, 1.1f);
        break;
    case 2:
        roof.shapeKind = RoofShapeKind::Dome;
        roof.height = NextRange(generator, 1.5f, std::min(3.4f, maxHeight));
        break;
    case 3:
        roof.shapeKind = RoofShapeKind::Pyramid;
        roof.height = NextRange(generator, 1.9f, std::min(3.8f, maxHeight));
        break;
    case 4:
        roof.shapeKind = RoofShapeKind::Wave;
        roof.height = NextRange(generator, 1.6f, std::min(3.1f, maxHeight));
        roof.primaryVariation = NextRange(generator, 1.25f, 2.25f);
        roof.secondaryVariation = NextRange(generator, 0.9f, 1.8f);
        roof.phase = NextRange(generator, 0.0f, Pi * 2.0f);
        break;
    default:
        roof.shapeKind = RoofShapeKind::Shed;
        roof.height = NextRange(generator, 1.5f, std::min(3.3f, maxHeight));
        break;
    }

    return roof;
}

Simulation::Simulation(std::uint32_t seed)
    : random_(seed)
{
    state_.seed = seed;
    state_.roof = RoofDefinition::CreateFlat(BuildingBodyHeight, RoofWidth, RoofDepth, 0.8f);
    state_.camera.target = state_.roof.CenterPoint();
    RefreshButtons();
    RandomizeRoof();
}

void Simulation::SetViewportSize(int width, int height)
{
    viewportWidth_ = std::max(1, width);
    viewportHeight_ = std::max(1, height);
    RefreshButtons();
    RefreshHud();
}

void Simulation::SetRoof(const RoofDefinition& roof)
{
    state_.roof = roof;
    RebuildScene();
}

void Simulation::RandomizeRoof()
{
    state_.roof = RoofDefinition::CreateRandom(random_, BuildingBodyHeight, RoofWidth, RoofDepth, MaxRoofHeight);
    RebuildScene();
}

void Simulation::RedistributeCookies()
{
    RebuildScene();
}

void Simulation::QueueAction(UiAction action)
{
    state_.pendingAction = action;
    ProcessPendingAction();
}

void Simulation::OrbitFromPixels(float deltaX, float deltaY)
{
    state_.camera.Orbit(deltaX * MinOrbitYawSpeed, deltaY * MinOrbitPitchSpeed);
    RefreshHud();
}

void Simulation::ZoomFromInput(float zoomDelta)
{
    state_.camera.Zoom(zoomDelta);
    RefreshHud();
}

void Simulation::UpdateHover(float x, float y)
{
    for (auto& button : state_.buttons)
    {
        button.hovered = button.Contains(x, y);
    }
}

void Simulation::SetPressedAction(UiAction action)
{
    for (auto& button : state_.buttons)
    {
        button.pressed = button.action == action;
    }
}

UiAction Simulation::HitTestButton(float x, float y) const
{
    for (const auto& button : state_.buttons)
    {
        if (button.Contains(x, y))
        {
            return button.action;
        }
    }

    return UiAction::None;
}

const SimulationState& Simulation::State() const noexcept
{
    return state_;
}

std::uint64_t Simulation::SceneRevision() const noexcept
{
    return sceneRevision_;
}

void Simulation::ProcessPendingAction()
{
    const auto action = state_.pendingAction;
    state_.pendingAction = UiAction::None;

    switch (action)
    {
    case UiAction::RandomizeRoof:
        RandomizeRoof();
        break;
    case UiAction::RedistributeCookies:
        RedistributeCookies();
        break;
    case UiAction::None:
        break;
    }
}

void Simulation::RebuildScene()
{
    state_.cookies = CreateCookiePlacements(state_.roof);
    state_.cookieCount = static_cast<int>(state_.cookies.size());
    state_.camera.target = state_.roof.CenterPoint();

    state_.meshes.clear();
    state_.sceneObjects.clear();
    state_.meshes.reserve(5);
    state_.sceneObjects.reserve(5);

    state_.meshes.push_back(CreateGroundMesh());
    state_.meshes.push_back(CreateShadowMesh());
    state_.meshes.push_back(CreateBox({0.0f, BuildingBodyHeight * 0.5f, 0.0f}, RoofWidth, BuildingBodyHeight, RoofDepth, BodyColor));
    state_.meshes.push_back(state_.roof.BuildMesh(SurfaceGridDivisions, RoofColor));
    state_.meshes.push_back(BuildCookieMesh(state_.cookies));

    for (std::size_t index = 0; index < state_.meshes.size(); ++index)
    {
        state_.sceneObjects.push_back({index, {}, {1.0f, 1.0f, 1.0f, 1.0f}});
    }

    RefreshButtons();
    RefreshHud();
    ++sceneRevision_;
}

void Simulation::RefreshButtons()
{
    state_.buttons = {
        {UiAction::RandomizeRoof, "RANDOMIZE ROOF", 20.0f, 20.0f, 228.0f, 52.0f, false, false},
        {UiAction::RedistributeCookies, "REDISTRIBUTE COOKIES", 20.0f, 84.0f, 268.0f, 52.0f, false, false},
    };
}

void Simulation::RefreshHud()
{
    state_.hudLines.clear();
    state_.hudLines.push_back("ROOF  " + state_.roof.DisplayName());
    state_.hudLines.push_back("SPAN  10.0M X 10.0M");
    state_.hudLines.push_back("PEAK  " + FormatFixed(state_.roof.height, 1) + "M");
    state_.hudLines.push_back("COOKIES  " + std::to_string(state_.cookieCount));
    if (state_.roof.ridgeGap > 0.0f)
    {
        state_.hudLines.push_back("RIDGE  " + FormatFixed(state_.roof.ridgeGap, 2) + "M CLEAR");
    }
    state_.hudLines.push_back("PITCH  " + FormatFixed(ToDegrees(state_.camera.pitch), 0) + " DEG");
    state_.hudLines.push_back("ZOOM  " + FormatFixed(state_.camera.distance, 1) + "M");
}

std::vector<CookiePlacement> Simulation::CreateCookiePlacements(const RoofDefinition& roof)
{
    if (roof.shapeKind == RoofShapeKind::Gable)
    {
        return CreateGableCookiePlacements(roof);
    }

    return CreateSurfaceCookiePlacements(roof);
}

std::vector<CookiePlacement> Simulation::CreateSurfaceCookiePlacements(const RoofDefinition& roof)
{
    const auto widths = CreatePartition(roof.width, state_.cookieSpec.minWidth, state_.cookieSpec.maxWidth, random_);
    const auto depths = CreatePartition(roof.depth, state_.cookieSpec.minDepth, state_.cookieSpec.maxDepth, random_);
    const auto xCenters = GetSegmentCenters(widths, -roof.width * 0.5f);
    const auto zCenters = GetSegmentCenters(depths, -roof.depth * 0.5f);

    std::vector<CookiePlacement> placements;
    placements.reserve(widths.size() * depths.size());

    for (std::size_t xIndex = 0; xIndex < widths.size(); ++xIndex)
    {
        for (std::size_t zIndex = 0; zIndex < depths.size(); ++zIndex)
        {
            const auto width = widths[xIndex];
            const auto depth = depths[zIndex];
            const auto sample = roof.Sample(xCenters[xIndex], zCenters[zIndex]);
            const auto tangentX = ProjectedAxis(sample.normal, {1.0f, 0.0f, 0.0f});
            const auto tangentZ = Normalize(Cross(sample.normal, tangentX));
            const auto height = NextRange(random_, state_.cookieSpec.minHeight, state_.cookieSpec.maxHeight);
            const auto color = CookiePalette[static_cast<std::size_t>(NextIndex(random_, static_cast<int>(CookiePalette.size())))];
            placements.push_back(CreateCookiePlacement(sample, tangentX, tangentZ, width, depth, height, color));
        }
    }

    return placements;
}

std::vector<CookiePlacement> Simulation::CreateGableCookiePlacements(const RoofDefinition& roof)
{
    const auto depths = CreatePartition(roof.depth, state_.cookieSpec.minDepth, state_.cookieSpec.maxDepth, random_);
    const auto zCenters = GetSegmentCenters(depths, -roof.depth * 0.5f);
    std::vector<CookiePlacement> placements;

    for (int sideSign : {-1, 1})
    {
        const auto halfWidth = roof.width * 0.5f;
        const auto ridgeHalfGap = roof.ridgeGap * 0.5f;
        const auto eaveX = sideSign < 0 ? -halfWidth : halfWidth;
        const auto ridgeEdgeX = sideSign < 0 ? -ridgeHalfGap : ridgeHalfGap;
        const auto eavePoint = roof.Sample(eaveX, 0.0f).point;
        const auto ridgePoint = roof.Sample(ridgeEdgeX, 0.0f).point;
        auto slopeVector = ridgePoint - eavePoint;
        const auto slopeLength = Length(slopeVector);
        if (slopeLength <= state_.cookieSpec.minWidth)
        {
            continue;
        }

        slopeVector = Normalize(slopeVector);
        const auto slopeWidths = CreatePartition(slopeLength, state_.cookieSpec.minWidth, state_.cookieSpec.maxWidth, random_);
        const auto slopeCenters = GetSegmentCenters(slopeWidths, 0.0f);
        const Vec3 depthDirection{0.0f, 0.0f, 1.0f};
        const auto normal = roof.Sample((eaveX + ridgeEdgeX) * 0.5f, 0.0f).normal;

        for (std::size_t slopeIndex = 0; slopeIndex < slopeWidths.size(); ++slopeIndex)
        {
            for (std::size_t depthIndex = 0; depthIndex < depths.size(); ++depthIndex)
            {
                const auto surfacePoint = eavePoint + (slopeVector * slopeCenters[slopeIndex]) + (depthDirection * zCenters[depthIndex]);
                const SurfaceSample sample{surfacePoint, normal};
                const auto height = NextRange(random_, state_.cookieSpec.minHeight, state_.cookieSpec.maxHeight);
                const auto color = CookiePalette[static_cast<std::size_t>(NextIndex(random_, static_cast<int>(CookiePalette.size())))];
                placements.push_back(CreateCookiePlacement(sample, slopeVector, depthDirection, slopeWidths[slopeIndex], depths[depthIndex], height, color));
            }
        }
    }

    return placements;
}
} // namespace cotrx
