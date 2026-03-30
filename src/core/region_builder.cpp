#include "core/region_builder.hpp"

#include <algorithm>
#include <cmath>

namespace cotrx::core
{

namespace
{
bool PointInPolygon(const Vec2& point, const std::vector<Vec2>& polygon)
{
    if (polygon.size() < 3)
    {
        return false;
    }

    bool inside = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
    {
        const auto& a = polygon[i];
        const auto& b = polygon[j];
        const auto intersects = ((a.y > point.y) != (b.y > point.y)) &&
            (point.x < ((b.x - a.x) * (point.y - a.y) / ((b.y - a.y) + 1.0e-12f) + a.x));
        if (intersects)
        {
            inside = !inside;
        }
    }

    return inside;
}

const ContourLoop* FindLoopById(const std::vector<ContourLoop>& loops, const std::uint32_t loopId)
{
    const auto it = std::find_if(loops.begin(), loops.end(), [&](const ContourLoop& loop) {
        return loop.loopId == loopId;
    });

    if (it == loops.end())
    {
        return nullptr;
    }

    return &(*it);
}

std::vector<Region> BuildRegionsForKind(
    const std::vector<ContourLoop>& loops,
    const CutBoundaryKind boundaryKind,
    const RegionKind regionKind,
    const SliceEpsilon& epsilon)
{
    std::vector<const ContourLoop*> candidates;
    for (const auto& loop : loops)
    {
        if (loop.boundaryKind == boundaryKind &&
            std::abs(loop.signedArea) > epsilon.segmentLength * epsilon.segmentLength)
        {
            candidates.push_back(&loop);
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const ContourLoop* a, const ContourLoop* b) {
        return a->signedArea > b->signedArea;
    });

    std::vector<Region> regions;
    for (const auto* loop : candidates)
    {
        bool holeAttached = false;
        for (auto& region : regions)
        {
            const auto* outer = FindLoopById(loops, region.outerLoopId);
            if (outer != nullptr && PointInPolygon(loop->points2D.front(), outer->points2D))
            {
                region.holeLoopIds.push_back(loop->loopId);
                holeAttached = true;
                break;
            }
        }

        if (!holeAttached)
        {
            regions.push_back({
                static_cast<std::uint32_t>(regions.size()),
                regionKind,
                loop->loopId,
                {}});
        }
    }

    return regions;
}

} // namespace

RegionBuildResult BuildRegions(const ContourBuildResult& contour, const SliceEpsilon& epsilon)
{
    RegionBuildResult result;
    result.meatRegions = BuildRegionsForKind(contour.loops, CutBoundaryKind::Meat, RegionKind::Meat, epsilon);
    result.skinRegions = BuildRegionsForKind(contour.loops, CutBoundaryKind::Skin, RegionKind::Skin, epsilon);

    // SkinRegion = skin loops minus MeatRegion
    result.skinRegions.erase(
        std::remove_if(result.skinRegions.begin(), result.skinRegions.end(), [&](const Region& skinRegion) {
            const auto* skinOuter = FindLoopById(contour.loops, skinRegion.outerLoopId);
            if (skinOuter == nullptr)
            {
                return true;
            }

            for (const auto& meatRegion : result.meatRegions)
            {
                const auto* meatOuter = FindLoopById(contour.loops, meatRegion.outerLoopId);
                if (meatOuter != nullptr && PointInPolygon(skinOuter->points2D.front(), meatOuter->points2D))
                {
                    return true;
                }
            }

            return false;
        }),
        result.skinRegions.end());

    for (std::uint32_t i = 0; i < result.skinRegions.size(); ++i)
    {
        result.skinRegions[i].regionId = i;
    }

    return result;
}

void region_builder_module_anchor()
{
}

} // namespace cotrx::core
