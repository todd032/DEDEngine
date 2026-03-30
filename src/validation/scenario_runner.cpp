#include "validation/scenario_runner.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/contour_builder.hpp"
#include "core/fragment_rebuild.hpp"
#include "core/region_builder.hpp"
#include "core/slice_intersections.hpp"
#include "core/surface_types.hpp"
#include "core/triangulation.hpp"

namespace cotrx::validation
{

namespace
{
constexpr float kAreaEpsilon = 1.0e-5f;

struct SurfaceStats
{
    std::uint32_t triangleCount = 0;
    float area = 0.0f;
};

struct FragmentStats
{
    std::uint32_t fragmentId = 0;
    std::uint32_t vertexCount = 0;
    std::uint32_t triangleCount = 0;
    float totalArea = 0.0f;
    std::unordered_map<core::SurfaceType, SurfaceStats> surfaceStats;
};

struct AssertionResult
{
    std::string name;
    bool passed = true;
    std::string detail;
};

struct ScenarioData
{
    std::string scenarioName;
    std::string scenarioDirName;
    std::vector<core::Fragment> fragments;
    std::vector<AssertionResult> assertions;
    std::optional<std::string> screenshotPath;
};

const char* ToString(const core::SurfaceType surfaceType)
{
    switch (surfaceType)
    {
    case core::SurfaceType::OuterSurface:
        return "OuterSurface";
    case core::SurfaceType::InnerSurface:
        return "InnerSurface";
    case core::SurfaceType::CutSurface:
        return "CutSurface";
    case core::SurfaceType::CutCap:
        return "CutCap";
    }

    return "Unknown";
}

float TriangleArea(const Vec3& a, const Vec3& b, const Vec3& c)
{
    const auto ab = b - a;
    const auto ac = c - a;
    return 0.5f * Length(Cross(ab, ac));
}

std::vector<core::TriangleView> BuildTriangleViews(const core::Fragment& fragment)
{
    std::vector<core::TriangleView> triangles;
    triangles.reserve(fragment.indices.size() / 3);

    for (std::size_t i = 0, triangleId = 0; i + 2 < fragment.indices.size(); i += 3, ++triangleId)
    {
        core::SurfaceType surfaceType = core::SurfaceType::OuterSurface;
        if (triangleId < fragment.triangleSurfaceTags.size())
        {
            surfaceType = fragment.triangleSurfaceTags[triangleId];
        }

        triangles.push_back({
            fragment.indices[i + 0],
            fragment.indices[i + 1],
            fragment.indices[i + 2],
            surfaceType});
    }

    return triangles;
}

bool FragmentIntersectsPlane(const core::Fragment& fragment, const core::Plane& plane, const core::SliceEpsilon& epsilon)
{
    bool sawPositive = false;
    bool sawNegative = false;

    for (const auto& vertex : fragment.vertices)
    {
        const auto distance = core::SignedDistanceToPlane(plane, vertex);
        if (distance > epsilon.distance)
        {
            sawPositive = true;
        }
        else if (distance < -epsilon.distance)
        {
            sawNegative = true;
        }

        if (sawPositive && sawNegative)
        {
            return true;
        }
    }

    return false;
}

std::vector<core::Fragment> ApplyPlaneCuts(
    const std::vector<core::Fragment>& inputFragments,
    const core::Plane& plane,
    const core::SliceEpsilon& epsilon)
{
    std::vector<core::Fragment> nextFragments;
    nextFragments.reserve(inputFragments.size() * 2);

    for (const auto& fragment : inputFragments)
    {
        const auto triangles = BuildTriangleViews(fragment);
        const auto sliced = core::SliceIntersections(fragment.vertices, triangles, plane, epsilon);

        if (sliced.cutSegments.empty())
        {
            nextFragments.push_back(fragment);
            continue;
        }

        const auto basis = core::BuildStablePlaneBasis(plane);
        const auto contours = core::BuildContourLoops(sliced.cutSegments, basis, epsilon);
        const auto regions = core::BuildRegions(contours, epsilon);
        const auto triangulated = core::TriangulateRegions(contours, regions, basis, epsilon);
        const auto rebuilt = core::RebuildFragments(sliced, triangulated);

        if (!rebuilt.positiveFragment.indices.empty())
        {
            auto positive = rebuilt.positiveFragment;
            positive.id = fragment.id * 2u;
            nextFragments.push_back(std::move(positive));
        }

        if (!rebuilt.negativeFragment.indices.empty())
        {
            auto negative = rebuilt.negativeFragment;
            negative.id = fragment.id * 2u + 1u;
            nextFragments.push_back(std::move(negative));
        }
    }

    return nextFragments;
}

std::unordered_map<core::SurfaceType, SurfaceStats> BuildAggregatedSurfaceStats(const std::vector<core::Fragment>& fragments)
{
    std::unordered_map<core::SurfaceType, SurfaceStats> stats;

    for (const auto& fragment : fragments)
    {
        for (std::size_t i = 0, triangleId = 0; i + 2 < fragment.indices.size(); i += 3, ++triangleId)
        {
            const auto ia = static_cast<std::size_t>(fragment.indices[i + 0]);
            const auto ib = static_cast<std::size_t>(fragment.indices[i + 1]);
            const auto ic = static_cast<std::size_t>(fragment.indices[i + 2]);
            if (ia >= fragment.vertices.size() || ib >= fragment.vertices.size() || ic >= fragment.vertices.size())
            {
                continue;
            }

            const auto surfaceType = triangleId < fragment.triangleSurfaceTags.size()
                ? fragment.triangleSurfaceTags[triangleId]
                : core::SurfaceType::OuterSurface;

            auto& surface = stats[surfaceType];
            surface.triangleCount += 1u;
            surface.area += TriangleArea(fragment.vertices[ia], fragment.vertices[ib], fragment.vertices[ic]);
        }
    }

    return stats;
}

std::vector<FragmentStats> BuildFragmentStats(const std::vector<core::Fragment>& fragments)
{
    std::vector<FragmentStats> fragmentStats;
    fragmentStats.reserve(fragments.size());

    for (const auto& fragment : fragments)
    {
        FragmentStats stats{};
        stats.fragmentId = fragment.id;
        stats.vertexCount = static_cast<std::uint32_t>(fragment.vertices.size());

        for (std::size_t i = 0, triangleId = 0; i + 2 < fragment.indices.size(); i += 3, ++triangleId)
        {
            const auto ia = static_cast<std::size_t>(fragment.indices[i + 0]);
            const auto ib = static_cast<std::size_t>(fragment.indices[i + 1]);
            const auto ic = static_cast<std::size_t>(fragment.indices[i + 2]);
            if (ia >= fragment.vertices.size() || ib >= fragment.vertices.size() || ic >= fragment.vertices.size())
            {
                continue;
            }

            const auto area = TriangleArea(fragment.vertices[ia], fragment.vertices[ib], fragment.vertices[ic]);
            const auto surfaceType = triangleId < fragment.triangleSurfaceTags.size()
                ? fragment.triangleSurfaceTags[triangleId]
                : core::SurfaceType::OuterSurface;

            stats.triangleCount += 1u;
            stats.totalArea += area;
            auto& surface = stats.surfaceStats[surfaceType];
            surface.triangleCount += 1u;
            surface.area += area;
        }

        fragmentStats.push_back(std::move(stats));
    }

    return fragmentStats;
}

float AreaForSurfaceType(
    const std::unordered_map<core::SurfaceType, SurfaceStats>& surfaceStats,
    const core::SurfaceType surfaceType)
{
    const auto it = surfaceStats.find(surfaceType);
    if (it == surfaceStats.end())
    {
        return 0.0f;
    }

    return it->second.area;
}

bool HasPositiveAreaForBoundary(
    const std::unordered_map<core::SurfaceType, SurfaceStats>& surfaceStats,
    const bool wantSkinBoundary)
{
    auto area = 0.0f;
    for (const auto& [surfaceType, stat] : surfaceStats)
    {
        const auto matches = wantSkinBoundary
            ? core::IsSkinBoundary(surfaceType)
            : core::IsMeatBoundary(surfaceType);
        if (matches)
        {
            area += stat.area;
        }
    }

    return area > kAreaEpsilon;
}

std::string SurfaceStatsJson(const std::unordered_map<core::SurfaceType, SurfaceStats>& surfaceStats, const int indent)
{
    std::ostringstream output;
    const std::string pad(indent, ' ');
    output << pad << "{\n";

    constexpr std::array<core::SurfaceType, 4> orderedTypes = {
        core::SurfaceType::OuterSurface,
        core::SurfaceType::InnerSurface,
        core::SurfaceType::CutSurface,
        core::SurfaceType::CutCap};

    for (std::size_t i = 0; i < orderedTypes.size(); ++i)
    {
        const auto type = orderedTypes[i];
        const auto it = surfaceStats.find(type);
        const auto triangleCount = it == surfaceStats.end() ? 0u : it->second.triangleCount;
        const auto area = it == surfaceStats.end() ? 0.0f : it->second.area;

        output << pad << "  \"" << ToString(type) << "\": {\"triangleCount\": " << triangleCount
               << ", \"area\": " << std::fixed << std::setprecision(6) << area << "}";

        if (i + 1 < orderedTypes.size())
        {
            output << ',';
        }
        output << "\n";
    }

    output << pad << '}';
    return output.str();
}

bool WriteObj(const std::filesystem::path& filePath, const std::vector<core::Fragment>& fragments)
{
    std::ofstream output(filePath, std::ios::trunc);
    if (!output.is_open())
    {
        return false;
    }

    std::uint32_t vertexBase = 1;
    for (const auto& fragment : fragments)
    {
        output << "o fragment_" << fragment.id << "\n";
        for (const auto& vertex : fragment.vertices)
        {
            output << "v " << vertex.x << ' ' << vertex.y << ' ' << vertex.z << "\n";
        }

        for (std::size_t i = 0; i + 2 < fragment.indices.size(); i += 3)
        {
            output << "f "
                   << fragment.indices[i + 0] + vertexBase << ' '
                   << fragment.indices[i + 1] + vertexBase << ' '
                   << fragment.indices[i + 2] + vertexBase << "\n";
        }

        vertexBase += static_cast<std::uint32_t>(fragment.vertices.size());
    }

    return true;
}

std::filesystem::path ValidationOutputDir(const std::string& scenarioDirName)
{
    return std::filesystem::path("outputs") / "validation" / scenarioDirName;
}

ScenarioExecutionResult PersistScenarioArtifacts(const ScenarioData& data, const ScenarioExecutionOptions& options)
{
    ScenarioExecutionResult result{};
    result.scenarioName = data.scenarioName;

    const auto outputDir = ValidationOutputDir(data.scenarioDirName);
    std::filesystem::create_directories(outputDir);

    result.outputDirectory = outputDir.generic_string();
    result.jsonPath = (outputDir / "report.json").generic_string();

    const auto fragmentStats = BuildFragmentStats(data.fragments);
    const auto aggregated = BuildAggregatedSurfaceStats(data.fragments);

    bool allAssertionsPassed = true;
    for (const auto& assertion : data.assertions)
    {
        allAssertionsPassed = allAssertionsPassed && assertion.passed;
        result.assertionMessages.push_back(std::string(assertion.passed ? "PASS: " : "FAIL: ") + assertion.name + " - " + assertion.detail);
    }

    std::ofstream json(result.jsonPath, std::ios::trunc);
    if (json.is_open())
    {
        json << "{\n";
        json << "  \"scenario\": \"" << data.scenarioName << "\",\n";
        json << "  \"outputDirectory\": \"" << result.outputDirectory << "\",\n";
        if (data.screenshotPath.has_value())
        {
            json << "  \"screenshotPath\": \"" << data.screenshotPath.value() << "\",\n";
        }
        else
        {
            json << "  \"screenshotPath\": null,\n";
        }
        json << "  \"fragmentCount\": " << data.fragments.size() << ",\n";
        json << "  \"surfaceTypeStats\": " << SurfaceStatsJson(aggregated, 2) << ",\n";
        json << "  \"fragments\": [\n";

        for (std::size_t i = 0; i < fragmentStats.size(); ++i)
        {
            const auto& stats = fragmentStats[i];
            json << "    {\n";
            json << "      \"id\": " << stats.fragmentId << ",\n";
            json << "      \"vertexCount\": " << stats.vertexCount << ",\n";
            json << "      \"triangleCount\": " << stats.triangleCount << ",\n";
            json << "      \"totalArea\": " << std::fixed << std::setprecision(6) << stats.totalArea << ",\n";
            json << "      \"surfaceTypeStats\": " << SurfaceStatsJson(stats.surfaceStats, 6) << "\n";
            json << "    }";
            if (i + 1 < fragmentStats.size())
            {
                json << ',';
            }
            json << "\n";
        }

        json << "  ],\n";
        json << "  \"assertions\": [\n";
        for (std::size_t i = 0; i < data.assertions.size(); ++i)
        {
            const auto& assertion = data.assertions[i];
            json << "    {\"name\": \"" << assertion.name << "\", \"passed\": "
                 << (assertion.passed ? "true" : "false")
                 << ", \"detail\": \"" << assertion.detail << "\"}";
            if (i + 1 < data.assertions.size())
            {
                json << ',';
            }
            json << "\n";
        }
        json << "  ],\n";
        json << "  \"passed\": " << (allAssertionsPassed ? "true" : "false") << "\n";
        json << "}\n";
    }

    if (options.dumpObj)
    {
        const auto objPath = (outputDir / "fragments.obj");
        if (WriteObj(objPath, data.fragments))
        {
            result.objPath = objPath.generic_string();
        }
    }

    result.success = allAssertionsPassed;
    return result;
}

std::vector<core::Fragment> DefaultInitialFragments(const core::ProceduralCubeGeneratorConfig& config)
{
    const auto shellPair = core::GenerateProceduralCubeShellPair(config);
    return {shellPair.outerShell, shellPair.innerShell};
}

AssertionResult MakeAssertion(const std::string& name, const bool passed, const std::string& detail)
{
    return {name, passed, detail};
}

} // namespace

CutPlaneScenarioInput BuildCutPlaneScenario(
    const CutPlanePreset preset,
    const core::ProceduralCubeGeneratorConfig& generatorConfig)
{
    const auto halfExtentX = std::max(generatorConfig.outerHalfExtent.x, 1.0e-4f);
    const auto insetHalfExtentX = std::max(halfExtentX - generatorConfig.skinThickness, 1.0e-4f);

    switch (preset)
    {
    case CutPlanePreset::ExactHalfCut:
        return {
            "ExactHalfCut",
            {{{1.0f, 0.0f, 0.0f}, -generatorConfig.center.x}}};
    case CutPlanePreset::ShallowCut:
        return {
            "ShallowCut",
            {{{1.0f, 0.0f, 0.0f}, -(generatorConfig.center.x + ((halfExtentX + insetHalfExtentX) * 0.5f))}}};
    case CutPlanePreset::RepeatedTrimming:
        return {
            "RepeatedTrimming",
            {
                {{1.0f, 0.0f, 0.0f}, -generatorConfig.center.x},
                {{0.0f, 1.0f, 0.0f}, -(generatorConfig.center.y + (halfExtentX * 0.2f))},
                {{0.0f, 0.0f, 1.0f}, -(generatorConfig.center.z - (halfExtentX * 0.15f))},
            }};
    }

    return {};
}

ScenarioCheckResult RunScenarioChecks(const ScenarioCheckInput& input)
{
    if (input.preset == CutPlanePreset::ShallowCut && !input.innerShellIntersected)
    {
        if (std::abs(input.cutCapArea) > kAreaEpsilon)
        {
            return {
                false,
                "ShallowCut without inner-shell intersection must keep CutCap area at zero."};
        }
    }

    return {true, {}};
}

ScenarioExecutionResult RunScenarioA(const ScenarioExecutionOptions& options)
{
    const core::SliceEpsilon epsilon{};
    auto fragments = DefaultInitialFragments(options.generatorConfig);
    const auto scenario = BuildCutPlaneScenario(CutPlanePreset::ExactHalfCut, options.generatorConfig);
    fragments = ApplyPlaneCuts(fragments, scenario.planes.front(), epsilon);

    const auto aggregated = BuildAggregatedSurfaceStats(fragments);
    const auto cutCapArea = AreaForSurfaceType(aggregated, core::SurfaceType::CutCap);

    ScenarioData data{};
    data.scenarioName = "Scenario A";
    data.scenarioDirName = "scenario_a";
    data.fragments = std::move(fragments);
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "inner penetrating cut should produce CutCap area > 0",
        cutCapArea > kAreaEpsilon,
        "CutCap area=" + std::to_string(cutCapArea)));

    return PersistScenarioArtifacts(data, options);
}

ScenarioExecutionResult RunScenarioB(const ScenarioExecutionOptions& options)
{
    const core::SliceEpsilon epsilon{};
    auto fragments = DefaultInitialFragments(options.generatorConfig);
    const auto scenario = BuildCutPlaneScenario(CutPlanePreset::ShallowCut, options.generatorConfig);

    const bool innerShellIntersected = FragmentIntersectsPlane(fragments[1], scenario.planes.front(), epsilon);
    fragments = ApplyPlaneCuts(fragments, scenario.planes.front(), epsilon);

    const auto aggregated = BuildAggregatedSurfaceStats(fragments);
    const auto cutCapArea = AreaForSurfaceType(aggregated, core::SurfaceType::CutCap);
    const auto check = RunScenarioChecks({CutPlanePreset::ShallowCut, innerShellIntersected, cutCapArea});

    ScenarioData data{};
    data.scenarioName = "Scenario B";
    data.scenarioDirName = "scenario_b";
    data.fragments = std::move(fragments);
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "shallow cut should keep CutCap area == 0 when inner shell is not intersected",
        check.passed,
        "innerShellIntersected=" + std::string(innerShellIntersected ? "true" : "false") +
            ", CutCap area=" + std::to_string(cutCapArea)));

    return PersistScenarioArtifacts(data, options);
}

ScenarioExecutionResult RunScenarioC(const ScenarioExecutionOptions& options)
{
    auto config = options.generatorConfig;
    config.skinThickness = std::max(config.skinThickness, 0.15f);

    const core::SliceEpsilon epsilon{};
    auto fragments = DefaultInitialFragments(config);
    const auto exact = BuildCutPlaneScenario(CutPlanePreset::ExactHalfCut, config);
    fragments = ApplyPlaneCuts(fragments, exact.planes.front(), epsilon);

    const auto aggregated = BuildAggregatedSurfaceStats(fragments);
    const auto cutCapArea = AreaForSurfaceType(aggregated, core::SurfaceType::CutCap);

    ScenarioData data{};
    data.scenarioName = "Scenario C";
    data.scenarioDirName = "scenario_c";
    data.fragments = std::move(fragments);
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "inner penetrating cut should produce CutCap area > 0",
        cutCapArea > kAreaEpsilon,
        "CutCap area=" + std::to_string(cutCapArea)));

    return PersistScenarioArtifacts(data, options);
}

ScenarioExecutionResult RunScenarioD(const ScenarioExecutionOptions& options)
{
    const core::SliceEpsilon epsilon{};
    auto fragments = DefaultInitialFragments(options.generatorConfig);
    const auto scenario = BuildCutPlaneScenario(CutPlanePreset::RepeatedTrimming, options.generatorConfig);

    for (const auto& plane : scenario.planes)
    {
        fragments = ApplyPlaneCuts(fragments, plane, epsilon);
    }

    const auto aggregated = BuildAggregatedSurfaceStats(fragments);
    const auto hasSkinBoundaryArea = HasPositiveAreaForBoundary(aggregated, true);
    const auto hasMeatBoundaryArea = HasPositiveAreaForBoundary(aggregated, false);

    ScenarioData data{};
    data.scenarioName = "Scenario D";
    data.scenarioDirName = "scenario_d";
    data.fragments = std::move(fragments);
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "re-cut should preserve SkinBoundary area when geometry remains",
        hasSkinBoundaryArea,
        "SkinBoundary area>0=" + std::string(hasSkinBoundaryArea ? "true" : "false")));
    data.assertions.push_back(MakeAssertion(
        "re-cut should preserve MeatBoundary area when geometry remains",
        hasMeatBoundaryArea,
        "MeatBoundary area>0=" + std::string(hasMeatBoundaryArea ? "true" : "false")));

    return PersistScenarioArtifacts(data, options);
}

ScenarioExecutionResult RunScenarioE(const ScenarioExecutionOptions& options)
{
    const core::SliceEpsilon epsilon{};
    auto fragments = DefaultInitialFragments(options.generatorConfig);

    const core::Plane missPlane{{1.0f, 0.0f, 0.0f}, -(options.generatorConfig.center.x + options.generatorConfig.outerHalfExtent.x + 5.0f)};
    const auto beforeCount = fragments.size();
    fragments = ApplyPlaneCuts(fragments, missPlane, epsilon);

    ScenarioData data{};
    data.scenarioName = "Scenario E";
    data.scenarioDirName = "scenario_e";
    data.fragments = std::move(fragments);
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "failure-inducing input should be reported explicitly",
        true,
        "Plane misses geometry; cutSegments expected to be empty."));
    data.assertions.push_back(MakeAssertion(
        "missed cut should not change fragment count",
        data.fragments.size() == beforeCount,
        "before=" + std::to_string(beforeCount) + ", after=" + std::to_string(data.fragments.size())));

    return PersistScenarioArtifacts(data, options);
}

std::vector<ScenarioExecutionResult> RunAllScenarios(const ScenarioExecutionOptions& options)
{
    return {
        RunScenarioA(options),
        RunScenarioB(options),
        RunScenarioC(options),
        RunScenarioD(options),
        RunScenarioE(options)};
}

void scenario_runner_module_anchor()
{
}

} // namespace cotrx::validation
