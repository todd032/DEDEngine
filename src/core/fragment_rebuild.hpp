#pragma once

#include "core/mesh_types.hpp"
#include "core/slice_intersections.hpp"
#include "core/triangulation.hpp"

namespace cotrx::core
{

struct FragmentRebuildResult
{
    Fragment positiveFragment;
    Fragment negativeFragment;
};

FragmentRebuildResult RebuildFragments(
    const SliceIntersectionsResult& sliced,
    const TriangulationResult& triangulated);

void fragment_rebuild_module_anchor();

} // namespace cotrx::core
