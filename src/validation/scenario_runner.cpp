#include "validation/scenario_runner.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
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

struct PlaneFragmentSectionStats
{
    std::uint32_t fragmentId = 0;
    std::uint32_t nearTriangleCount = 0;
    float nearArea = 0.0f;
    std::uint32_t mixedComponents = 0;
    bool hasConnectedCutSurfaceAndCutCap = false;
    std::unordered_map<core::SurfaceType, SurfaceStats> surfaceStats;
};

struct PlaneSectionStats
{
    std::uint32_t stepIndex = 0;
    core::Plane plane{};
    std::uint32_t nearTriangleCount = 0;
    float nearArea = 0.0f;
    std::uint32_t mixedComponentFragments = 0;
    std::unordered_map<core::SurfaceType, SurfaceStats> surfaceStats;
    std::vector<PlaneFragmentSectionStats> fragmentStats;
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
    std::vector<PlaneSectionStats> stepSectionStats;
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

Vec3 TriangleCentroid(const Vec3& a, const Vec3& b, const Vec3& c)
{
    return {(a.x + b.x + c.x) / 3.0f, (a.y + b.y + c.y) / 3.0f, (a.z + b.z + c.z) / 3.0f};
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

PlaneSectionStats BuildPlaneSectionStats(
    const std::vector<core::Fragment>& fragments,
    const core::Plane& plane,
    const core::SliceEpsilon& epsilon,
    const std::uint32_t stepIndex)
{
    struct LocalTriangle
    {
        std::uint32_t ia = 0;
        std::uint32_t ib = 0;
        std::uint32_t ic = 0;
        core::SurfaceType surfaceType = core::SurfaceType::OuterSurface;
        float area = 0.0f;
    };

    constexpr float kSectionDistanceFloor = 1.0e-4f;
    const auto sectionDistance = std::max(kSectionDistanceFloor, epsilon.distance * 8.0f);

    PlaneSectionStats section{};
    section.stepIndex = stepIndex;
    section.plane = plane;

    for (const auto& fragment : fragments)
    {
        PlaneFragmentSectionStats fragmentSection{};
        fragmentSection.fragmentId = fragment.id;
        std::vector<LocalTriangle> nearTriangles;
        nearTriangles.reserve(fragment.indices.size() / 3);

        for (std::size_t i = 0, triangleId = 0; i + 2 < fragment.indices.size(); i += 3, ++triangleId)
        {
            const auto ia = static_cast<std::size_t>(fragment.indices[i + 0]);
            const auto ib = static_cast<std::size_t>(fragment.indices[i + 1]);
            const auto ic = static_cast<std::size_t>(fragment.indices[i + 2]);
            if (ia >= fragment.vertices.size() || ib >= fragment.vertices.size() || ic >= fragment.vertices.size())
            {
                continue;
            }

            const auto& a = fragment.vertices[ia];
            const auto& b = fragment.vertices[ib];
            const auto& c = fragment.vertices[ic];
            const auto centroid = TriangleCentroid(a, b, c);
            if (std::abs(core::SignedDistanceToPlane(plane, centroid)) > sectionDistance)
            {
                continue;
            }

            const auto surfaceType = triangleId < fragment.triangleSurfaceTags.size()
                ? fragment.triangleSurfaceTags[triangleId]
                : core::SurfaceType::OuterSurface;
            const auto area = TriangleArea(a, b, c);

            nearTriangles.push_back({
                static_cast<std::uint32_t>(fragment.indices[i + 0]),
                static_cast<std::uint32_t>(fragment.indices[i + 1]),
                static_cast<std::uint32_t>(fragment.indices[i + 2]),
                surfaceType,
                area});

            fragmentSection.nearTriangleCount += 1u;
            fragmentSection.nearArea += area;
            auto& localSurface = fragmentSection.surfaceStats[surfaceType];
            localSurface.triangleCount += 1u;
            localSurface.area += area;
        }

        if (!nearTriangles.empty())
        {
            std::unordered_map<std::uint64_t, std::vector<std::size_t>> edgeToTriangles;
            for (std::size_t triIndex = 0; triIndex < nearTriangles.size(); ++triIndex)
            {
                const auto& t = nearTriangles[triIndex];
                const auto addEdge = [&edgeToTriangles, triIndex](const std::uint32_t v0, const std::uint32_t v1)
                {
                    const auto lo = std::min(v0, v1);
                    const auto hi = std::max(v0, v1);
                    const auto key = (static_cast<std::uint64_t>(lo) << 32u) | static_cast<std::uint64_t>(hi);
                    edgeToTriangles[key].push_back(triIndex);
                };

                addEdge(t.ia, t.ib);
                addEdge(t.ib, t.ic);
                addEdge(t.ic, t.ia);
            }

            std::vector<std::vector<std::size_t>> adjacency(nearTriangles.size());
            for (const auto& [_, triList] : edgeToTriangles)
            {
                for (std::size_t i = 0; i < triList.size(); ++i)
                {
                    for (std::size_t j = i + 1; j < triList.size(); ++j)
                    {
                        adjacency[triList[i]].push_back(triList[j]);
                        adjacency[triList[j]].push_back(triList[i]);
                    }
                }
            }

            std::vector<bool> visited(nearTriangles.size(), false);
            for (std::size_t triIndex = 0; triIndex < nearTriangles.size(); ++triIndex)
            {
                if (visited[triIndex])
                {
                    continue;
                }

                std::vector<std::size_t> stack{triIndex};
                visited[triIndex] = true;
                float cutSurfaceArea = 0.0f;
                float cutCapArea = 0.0f;

                while (!stack.empty())
                {
                    const auto current = stack.back();
                    stack.pop_back();

                    const auto& triangle = nearTriangles[current];
                    if (triangle.surfaceType == core::SurfaceType::CutSurface)
                    {
                        cutSurfaceArea += triangle.area;
                    }
                    else if (triangle.surfaceType == core::SurfaceType::CutCap)
                    {
                        cutCapArea += triangle.area;
                    }

                    for (const auto neighbor : adjacency[current])
                    {
                        if (!visited[neighbor])
                        {
                            visited[neighbor] = true;
                            stack.push_back(neighbor);
                        }
                    }
                }

                if (cutSurfaceArea > kAreaEpsilon && cutCapArea > kAreaEpsilon)
                {
                    fragmentSection.mixedComponents += 1u;
                    fragmentSection.hasConnectedCutSurfaceAndCutCap = true;
                }
            }
        }

        if (fragmentSection.nearTriangleCount > 0u)
        {
            section.nearTriangleCount += fragmentSection.nearTriangleCount;
            section.nearArea += fragmentSection.nearArea;
            section.mixedComponentFragments += fragmentSection.hasConnectedCutSurfaceAndCutCap ? 1u : 0u;
            for (const auto& [surfaceType, stat] : fragmentSection.surfaceStats)
            {
                auto& aggregateSurface = section.surfaceStats[surfaceType];
                aggregateSurface.triangleCount += stat.triangleCount;
                aggregateSurface.area += stat.area;
            }
            section.fragmentStats.push_back(std::move(fragmentSection));
        }
    }

    return section;
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

std::string PlaneSectionStatsJson(const std::vector<PlaneSectionStats>& stats, const int indent)
{
    std::ostringstream output;
    const std::string pad(indent, ' ');
    output << pad << "[\n";

    for (std::size_t i = 0; i < stats.size(); ++i)
    {
        const auto& step = stats[i];
        output << pad << "  {\n";
        output << pad << "    \"step\": " << step.stepIndex << ",\n";
        output << pad << "    \"plane\": {\"normal\": ["
               << std::fixed << std::setprecision(6)
               << step.plane.normal.x << ", " << step.plane.normal.y << ", " << step.plane.normal.z
               << "], \"distance\": " << step.plane.distance << "},\n";
        output << pad << "    \"nearTriangleCount\": " << step.nearTriangleCount << ",\n";
        output << pad << "    \"nearArea\": " << std::fixed << std::setprecision(6) << step.nearArea << ",\n";
        output << pad << "    \"mixedComponentFragments\": " << step.mixedComponentFragments << ",\n";
        output << pad << "    \"surfaceTypeStats\": " << SurfaceStatsJson(step.surfaceStats, indent + 4) << ",\n";
        output << pad << "    \"fragments\": [\n";
        for (std::size_t f = 0; f < step.fragmentStats.size(); ++f)
        {
            const auto& fragment = step.fragmentStats[f];
            output << pad << "      {\n";
            output << pad << "        \"id\": " << fragment.fragmentId << ",\n";
            output << pad << "        \"nearTriangleCount\": " << fragment.nearTriangleCount << ",\n";
            output << pad << "        \"nearArea\": " << std::fixed << std::setprecision(6) << fragment.nearArea << ",\n";
            output << pad << "        \"mixedComponents\": " << fragment.mixedComponents << ",\n";
            output << pad << "        \"hasConnectedCutSurfaceAndCutCap\": "
                   << (fragment.hasConnectedCutSurfaceAndCutCap ? "true" : "false") << ",\n";
            output << pad << "        \"surfaceTypeStats\": " << SurfaceStatsJson(fragment.surfaceStats, indent + 8) << "\n";
            output << pad << "      }";
            if (f + 1 < step.fragmentStats.size())
            {
                output << ',';
            }
            output << "\n";
        }
        output << pad << "    ]\n";
        output << pad << "  }";
        if (i + 1 < stats.size())
        {
            output << ',';
        }
        output << "\n";
    }

    output << pad << "]";
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
        json << "  \"stepSectionStats\": " << PlaneSectionStatsJson(data.stepSectionStats, 2) << ",\n";
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
    const auto shallowCutX = generatorConfig.center.x + ((halfExtentX + insetHalfExtentX) * 0.5f);
    const auto progressiveX0 = generatorConfig.center.x + (halfExtentX * 0.60f);
    const auto progressiveX1 = generatorConfig.center.x + (halfExtentX * 0.72f);
    const auto progressiveX2 = generatorConfig.center.x + (halfExtentX * 0.82f);
    const auto progressiveX3 = generatorConfig.center.x + (halfExtentX * 0.90f);

    switch (preset)
    {
    case CutPlanePreset::InitialVisibility:
        return {"InitialVisibility", {}};
    case CutPlanePreset::VerticalHalfCut:
        return {
            "VerticalHalfCut",
            {{{1.0f, 0.0f, 0.0f}, -generatorConfig.center.x}}};
    case CutPlanePreset::VerticalThenHorizontalCut:
        return {
            "VerticalThenHorizontalCut",
            {
                {{1.0f, 0.0f, 0.0f}, -generatorConfig.center.x},
                {{0.0f, 1.0f, 0.0f}, -generatorConfig.center.y},
            }};
    case CutPlanePreset::ProgressiveSkinRemoval:
        return {
            "ProgressiveSkinRemoval",
            {
                {{1.0f, 0.0f, 0.0f}, -progressiveX0},
                {{1.0f, 0.0f, 0.0f}, -progressiveX1},
                {{1.0f, 0.0f, 0.0f}, -progressiveX2},
                {{1.0f, 0.0f, 0.0f}, -progressiveX3},
            }};
    case CutPlanePreset::ShallowCutNoMeatExposure:
        return {
            "ShallowCutNoMeatExposure",
            {{{1.0f, 0.0f, 0.0f}, -shallowCutX}}};
    }

    return {};
}

ScenarioCheckResult RunScenarioChecks(const ScenarioCheckInput& input)
{
    if (input.preset == CutPlanePreset::ShallowCutNoMeatExposure && !input.innerShellIntersected)
    {
        if (std::abs(input.cutCapArea) > kAreaEpsilon)
        {
            return {
                false,
                "ShallowCutNoMeatExposure without inner-shell intersection must keep CutCap area at zero."};
        }
    }

    return {true, {}};
}

ScenarioExecutionResult RunScenarioA(const ScenarioExecutionOptions& options)
{
    auto fragments = DefaultInitialFragments(options.generatorConfig);
    const auto aggregated = BuildAggregatedSurfaceStats(fragments);
    const auto outerArea = AreaForSurfaceType(aggregated, core::SurfaceType::OuterSurface);
    const auto cutSurfaceArea = AreaForSurfaceType(aggregated, core::SurfaceType::CutSurface);
    const auto cutCapArea = AreaForSurfaceType(aggregated, core::SurfaceType::CutCap);

    ScenarioData data{};
    data.scenarioName = "Scenario A: InitialVisibility";
    data.scenarioDirName = "scenario_a";
    data.fragments = std::move(fragments);
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "initial visibility exposes only OuterSurface on observable boundary",
        (outerArea > kAreaEpsilon) && (cutSurfaceArea <= kAreaEpsilon) && (cutCapArea <= kAreaEpsilon),
        "OuterSurface area=" + std::to_string(outerArea) + ", CutSurface area=" + std::to_string(cutSurfaceArea) +
            ", CutCap area=" + std::to_string(cutCapArea)));

    return PersistScenarioArtifacts(data, options);
}

ScenarioExecutionResult RunScenarioB(const ScenarioExecutionOptions& options)
{
    const core::SliceEpsilon epsilon{};
    auto fragments = DefaultInitialFragments(options.generatorConfig);
    const auto scenario = BuildCutPlaneScenario(CutPlanePreset::VerticalHalfCut, options.generatorConfig);

    fragments = ApplyPlaneCuts(fragments, scenario.planes.front(), epsilon);
    const auto stepSectionStats = std::vector<PlaneSectionStats>{
        BuildPlaneSectionStats(fragments, scenario.planes.front(), epsilon, 0u)};

    const auto aggregated = BuildAggregatedSurfaceStats(fragments);
    const auto cutSurfaceArea = AreaForSurfaceType(aggregated, core::SurfaceType::CutSurface);
    const auto cutCapArea = AreaForSurfaceType(aggregated, core::SurfaceType::CutCap);
    const auto sectionCutSurfaceArea = AreaForSurfaceType(stepSectionStats.front().surfaceStats, core::SurfaceType::CutSurface);
    const auto sectionCutCapArea = AreaForSurfaceType(stepSectionStats.front().surfaceStats, core::SurfaceType::CutCap);
    const auto sectionCutBoundaryArea = sectionCutSurfaceArea + sectionCutCapArea;
    const auto sectionCutSurfaceRatio = sectionCutBoundaryArea > kAreaEpsilon ? (sectionCutSurfaceArea / sectionCutBoundaryArea) : 0.0f;
    const auto sectionCutCapRatio = sectionCutBoundaryArea > kAreaEpsilon ? (sectionCutCapArea / sectionCutBoundaryArea) : 0.0f;
    const auto cutBoundaryQualityPass =
        (sectionCutSurfaceArea > kAreaEpsilon) && (sectionCutCapArea > kAreaEpsilon) &&
        (sectionCutSurfaceRatio >= 0.05f) && (sectionCutCapRatio >= 0.05f) &&
        (stepSectionStats.front().mixedComponentFragments > 0u);

    ScenarioData data{};
    data.scenarioName = "Scenario B: VerticalHalfCut";
    data.scenarioDirName = "scenario_b";
    data.fragments = std::move(fragments);
    data.stepSectionStats = stepSectionStats;
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "vertical half cut should produce both CutSurface(skin boundary) and CutCap(meat boundary)",
        (cutSurfaceArea > kAreaEpsilon) && (cutCapArea > kAreaEpsilon),
        "CutSurface area=" + std::to_string(cutSurfaceArea) + ", CutCap area=" + std::to_string(cutCapArea)));
    data.assertions.push_back(MakeAssertion(
        "cross-section should keep skin/meat co-existence with minimum ratio and connectivity",
        cutBoundaryQualityPass,
        "section CutSurface ratio=" + std::to_string(sectionCutSurfaceRatio) +
            ", CutCap ratio=" + std::to_string(sectionCutCapRatio) +
            ", connected mixed fragments=" + std::to_string(stepSectionStats.front().mixedComponentFragments)));

    return PersistScenarioArtifacts(data, options);
}

ScenarioExecutionResult RunScenarioC(const ScenarioExecutionOptions& options)
{
    const core::SliceEpsilon epsilon{};
    auto fragments = DefaultInitialFragments(options.generatorConfig);
    const auto scenario = BuildCutPlaneScenario(CutPlanePreset::VerticalThenHorizontalCut, options.generatorConfig);
    std::vector<PlaneSectionStats> stepSectionStats;
    stepSectionStats.reserve(scenario.planes.size());
    for (const auto& plane : scenario.planes)
    {
        fragments = ApplyPlaneCuts(fragments, plane, epsilon);
        stepSectionStats.push_back(BuildPlaneSectionStats(
            fragments,
            plane,
            epsilon,
            static_cast<std::uint32_t>(stepSectionStats.size())));
    }

    const auto fragmentStats = BuildFragmentStats(fragments);
    auto allBoundaryMixValid = true;
    auto violatingCount = 0u;
    for (const auto& stat : fragmentStats)
    {
        const auto cutSurfaceArea = AreaForSurfaceType(stat.surfaceStats, core::SurfaceType::CutSurface);
        const auto cutCapArea = AreaForSurfaceType(stat.surfaceStats, core::SurfaceType::CutCap);
        if ((cutSurfaceArea > kAreaEpsilon || cutCapArea > kAreaEpsilon) &&
            ((cutSurfaceArea <= kAreaEpsilon) || (cutCapArea <= kAreaEpsilon)))
        {
            allBoundaryMixValid = false;
            ++violatingCount;
        }
    }

    ScenarioData data{};
    data.scenarioName = "Scenario C: VerticalThenHorizontalCut";
    data.scenarioDirName = "scenario_c";
    data.fragments = std::move(fragments);
    data.stepSectionStats = stepSectionStats;
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "after second cut, each fragment that has cut boundary keeps both CutSurface and CutCap",
        allBoundaryMixValid,
        "violating fragments=" + std::to_string(violatingCount) + ", total fragments=" + std::to_string(fragmentStats.size())));
    if (!data.stepSectionStats.empty())
    {
        const auto& lastStep = data.stepSectionStats.back();
        const auto sectionCutSurfaceArea = AreaForSurfaceType(lastStep.surfaceStats, core::SurfaceType::CutSurface);
        const auto sectionCutCapArea = AreaForSurfaceType(lastStep.surfaceStats, core::SurfaceType::CutCap);
        const auto sectionCutBoundaryArea = sectionCutSurfaceArea + sectionCutCapArea;
        const auto sectionCutSurfaceRatio = sectionCutBoundaryArea > kAreaEpsilon ? (sectionCutSurfaceArea / sectionCutBoundaryArea) : 0.0f;
        const auto sectionCutCapRatio = sectionCutBoundaryArea > kAreaEpsilon ? (sectionCutCapArea / sectionCutBoundaryArea) : 0.0f;
        const auto sectionQualityPass =
            (sectionCutSurfaceArea > kAreaEpsilon) && (sectionCutCapArea > kAreaEpsilon) &&
            (sectionCutSurfaceRatio >= 0.05f) && (sectionCutCapRatio >= 0.05f) &&
            (lastStep.mixedComponentFragments > 0u);
        data.assertions.push_back(MakeAssertion(
            "second-cut section should preserve CutSurface/CutCap ratio and connectedness",
            sectionQualityPass,
            "step=1, CutSurface ratio=" + std::to_string(sectionCutSurfaceRatio) +
                ", CutCap ratio=" + std::to_string(sectionCutCapRatio) +
                ", connected mixed fragments=" + std::to_string(lastStep.mixedComponentFragments)));
    }

    return PersistScenarioArtifacts(data, options);
}

ScenarioExecutionResult RunScenarioD(const ScenarioExecutionOptions& options)
{
    const core::SliceEpsilon epsilon{};
    auto fragments = DefaultInitialFragments(options.generatorConfig);
    const auto scenario = BuildCutPlaneScenario(CutPlanePreset::ProgressiveSkinRemoval, options.generatorConfig);
    std::vector<PlaneSectionStats> stepSectionStats;
    stepSectionStats.reserve(scenario.planes.size());

    for (const auto& plane : scenario.planes)
    {
        fragments = ApplyPlaneCuts(fragments, plane, epsilon);
        stepSectionStats.push_back(BuildPlaneSectionStats(
            fragments,
            plane,
            epsilon,
            static_cast<std::uint32_t>(stepSectionStats.size())));
    }

    const auto fragmentStats = BuildFragmentStats(fragments);
    bool sawSkinErasedRegion = false;
    for (const auto& stat : fragmentStats)
    {
        const auto outerArea = AreaForSurfaceType(stat.surfaceStats, core::SurfaceType::OuterSurface);
        const auto cutSurfaceArea = AreaForSurfaceType(stat.surfaceStats, core::SurfaceType::CutSurface);
        if (outerArea <= kAreaEpsilon && cutSurfaceArea <= kAreaEpsilon)
        {
            sawSkinErasedRegion = true;
            break;
        }
    }

    ScenarioData data{};
    data.scenarioName = "Scenario D: ProgressiveSkinRemoval";
    data.scenarioDirName = "scenario_d";
    data.fragments = std::move(fragments);
    data.stepSectionStats = stepSectionStats;
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "progressive skin removal should converge to region with OuterSurface/CutSurface ~= 0",
        sawSkinErasedRegion,
        "skin-erased fragment present=" + std::string(sawSkinErasedRegion ? "true" : "false")));
    bool monotonicSkinBoundary = true;
    bool sawZero = false;
    float prevSkinBoundaryArea = std::numeric_limits<float>::infinity();
    for (const auto& step : data.stepSectionStats)
    {
        const auto skinBoundaryArea = AreaForSurfaceType(step.surfaceStats, core::SurfaceType::CutSurface);
        if (sawZero && skinBoundaryArea > kAreaEpsilon)
        {
            monotonicSkinBoundary = false;
            break;
        }
        if (skinBoundaryArea <= kAreaEpsilon)
        {
            sawZero = true;
        }
        if (skinBoundaryArea > prevSkinBoundaryArea + kAreaEpsilon)
        {
            monotonicSkinBoundary = false;
            break;
        }
        prevSkinBoundaryArea = skinBoundaryArea;
    }
    data.assertions.push_back(MakeAssertion(
        "progressive cuts should keep section skin-boundary(CutSurface) area monotonic non-increasing",
        monotonicSkinBoundary,
        "step count=" + std::to_string(data.stepSectionStats.size()) +
            ", reached zero=" + std::string(sawZero ? "true" : "false")));

    return PersistScenarioArtifacts(data, options);
}

ScenarioExecutionResult RunScenarioE(const ScenarioExecutionOptions& options)
{
    const core::SliceEpsilon epsilon{};
    auto fragments = DefaultInitialFragments(options.generatorConfig);
    const auto scenario = BuildCutPlaneScenario(CutPlanePreset::ShallowCutNoMeatExposure, options.generatorConfig);
    const bool innerShellIntersected = FragmentIntersectsPlane(fragments[1], scenario.planes.front(), epsilon);
    fragments = ApplyPlaneCuts(fragments, scenario.planes.front(), epsilon);
    const auto stepSectionStats = std::vector<PlaneSectionStats>{
        BuildPlaneSectionStats(fragments, scenario.planes.front(), epsilon, 0u)};

    const auto fragmentStats = BuildFragmentStats(fragments);
    bool allFragmentsNoCutCap = true;
    for (const auto& stat : fragmentStats)
    {
        if (AreaForSurfaceType(stat.surfaceStats, core::SurfaceType::CutCap) > kAreaEpsilon)
        {
            allFragmentsNoCutCap = false;
            break;
        }
    }

    const auto aggregated = BuildAggregatedSurfaceStats(fragments);
    const auto cutCapArea = AreaForSurfaceType(aggregated, core::SurfaceType::CutCap);
    const auto check = RunScenarioChecks({CutPlanePreset::ShallowCutNoMeatExposure, innerShellIntersected, cutCapArea});

    ScenarioData data{};
    data.scenarioName = "Scenario E: ShallowCutNoMeatExposure";
    data.scenarioDirName = "scenario_e";
    data.fragments = std::move(fragments);
    data.stepSectionStats = stepSectionStats;
    data.screenshotPath = options.screenshotPath;
    data.assertions.push_back(MakeAssertion(
        "shallow cut should keep CutCap == 0 for every fragment when inner shell is not intersected",
        check.passed && allFragmentsNoCutCap,
        "innerShellIntersected=" + std::string(innerShellIntersected ? "true" : "false") +
            ", aggregated CutCap area=" + std::to_string(cutCapArea)));

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
