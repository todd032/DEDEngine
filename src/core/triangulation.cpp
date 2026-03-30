#include "core/triangulation.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace cotrx::core
{

namespace
{
float SignedArea2(const Vec2& a, const Vec2& b, const Vec2& c)
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool IsPointInTriangle(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c)
{
    const auto s1 = SignedArea2(a, b, p);
    const auto s2 = SignedArea2(b, c, p);
    const auto s3 = SignedArea2(c, a, p);
    const auto hasNegative = (s1 < 0.0f) || (s2 < 0.0f) || (s3 < 0.0f);
    const auto hasPositive = (s1 > 0.0f) || (s2 > 0.0f) || (s3 > 0.0f);
    return !(hasNegative && hasPositive);
}

void EarClipSimplePolygon(
    const std::vector<Vec2>& polygon,
    std::vector<std::array<std::uint32_t, 3>>& outTriangles,
    const SliceEpsilon& epsilon)
{
    if (polygon.size() < 3)
    {
        return;
    }

    std::vector<std::uint32_t> remaining;
    remaining.reserve(polygon.size());
    for (std::uint32_t i = 0; i < polygon.size(); ++i)
    {
        remaining.push_back(i);
    }

    std::uint32_t guard = 0;
    while (remaining.size() > 2 && guard++ < 4096)
    {
        bool clipped = false;
        for (std::size_t i = 0; i < remaining.size(); ++i)
        {
            const auto prev = remaining[(i + remaining.size() - 1) % remaining.size()];
            const auto curr = remaining[i];
            const auto next = remaining[(i + 1) % remaining.size()];

            const auto area = SignedArea2(polygon[prev], polygon[curr], polygon[next]);
            if (area <= epsilon.segmentLength)
            {
                continue;
            }

            bool containsPoint = false;
            for (const auto test : remaining)
            {
                if (test == prev || test == curr || test == next)
                {
                    continue;
                }

                if (IsPointInTriangle(polygon[test], polygon[prev], polygon[curr], polygon[next]))
                {
                    containsPoint = true;
                    break;
                }
            }

            if (containsPoint)
            {
                continue;
            }

            outTriangles.push_back({prev, curr, next});
            remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
            clipped = true;
            break;
        }

        if (!clipped)
        {
            break;
        }
    }
}

const ContourLoop* FindLoopById(const ContourBuildResult& contours, const std::uint32_t loopId)
{
    const auto it = std::find_if(contours.loops.begin(), contours.loops.end(), [&](const ContourLoop& loop) {
        return loop.loopId == loopId;
    });

    if (it == contours.loops.end())
    {
        return nullptr;
    }

    return &(*it);
}

TriangulatedPatch TriangulateOneRegion(
    const ContourBuildResult& contours,
    const Region& region,
    const PlaneBasis& basis,
    const SliceEpsilon& epsilon)
{
    TriangulatedPatch patch;
    patch.regionKind = region.kind;

    const auto* outer = FindLoopById(contours, region.outerLoopId);
    if (outer == nullptr)
    {
        return patch;
    }

    std::vector<std::array<std::uint32_t, 3>> triangles;
    EarClipSimplePolygon(outer->points2D, triangles, epsilon);

    patch.vertices.reserve(outer->points2D.size());
    for (const auto& point : outer->points2D)
    {
        patch.vertices.push_back(UnprojectPointFromPlane(basis, point));
    }

    for (const auto& triangle : triangles)
    {
        patch.indices.push_back(triangle[0]);
        patch.indices.push_back(triangle[1]);
        patch.indices.push_back(triangle[2]);
    }

    return patch;
}

} // namespace

TriangulationResult TriangulateRegions(
    const ContourBuildResult& contours,
    const RegionBuildResult& regions,
    const PlaneBasis& basis,
    const SliceEpsilon& epsilon)
{
    TriangulationResult result;

    for (const auto& skinRegion : regions.skinRegions)
    {
        auto patch = TriangulateOneRegion(contours, skinRegion, basis, epsilon);
        if (!patch.indices.empty())
        {
            result.cutSurfacePatches.push_back(std::move(patch));
        }
    }

    for (const auto& meatRegion : regions.meatRegions)
    {
        auto patch = TriangulateOneRegion(contours, meatRegion, basis, epsilon);
        if (!patch.indices.empty())
        {
            result.cutCapPatches.push_back(std::move(patch));
        }
    }

    return result;
}

void triangulation_module_anchor()
{
}

} // namespace cotrx::core
