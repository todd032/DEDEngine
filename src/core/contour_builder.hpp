#pragma once

#include <cstdint>
#include <vector>

#include "core/plane_math.hpp"
#include "core/slice_intersections.hpp"

namespace cotrx::core
{

struct ContourLoop
{
    std::uint32_t loopId = 0;
    CutBoundaryKind boundaryKind = CutBoundaryKind::Skin;
    std::vector<Vec3> points3D;
    std::vector<Vec2> points2D;
    float signedArea = 0.0f;
};

struct ContourBuildResult
{
    std::vector<ContourLoop> loops;
};

ContourBuildResult BuildContourLoops(
    const std::vector<CutSegment>& segments,
    const PlaneBasis& basis,
    const SliceEpsilon& epsilon);

void contour_builder_module_anchor();

} // namespace cotrx::core
