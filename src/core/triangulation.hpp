#pragma once

#include <cstdint>
#include <vector>

#include "core/region_builder.hpp"

namespace cotrx::core
{

struct TriangulatedPatch
{
    RegionKind regionKind = RegionKind::Skin;
    std::vector<Vec3> vertices;
    std::vector<std::uint32_t> indices;
};

struct TriangulationResult
{
    std::vector<TriangulatedPatch> cutSurfacePatches;
    std::vector<TriangulatedPatch> cutCapPatches;
};

TriangulationResult TriangulateRegions(
    const ContourBuildResult& contours,
    const RegionBuildResult& regions,
    const PlaneBasis& basis,
    const SliceEpsilon& epsilon);

void triangulation_module_anchor();

} // namespace cotrx::core
