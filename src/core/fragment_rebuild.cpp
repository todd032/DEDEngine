#include "core/fragment_rebuild.hpp"

#include <algorithm>

namespace cotrx::core
{

namespace
{
Bounds3 ComputeBounds(const std::vector<Vec3>& vertices)
{
    Bounds3 bounds{};
    if (vertices.empty())
    {
        return bounds;
    }

    bounds.min = vertices.front();
    bounds.max = vertices.front();

    for (const auto& vertex : vertices)
    {
        bounds.min.x = std::min(bounds.min.x, vertex.x);
        bounds.min.y = std::min(bounds.min.y, vertex.y);
        bounds.min.z = std::min(bounds.min.z, vertex.z);
        bounds.max.x = std::max(bounds.max.x, vertex.x);
        bounds.max.y = std::max(bounds.max.y, vertex.y);
        bounds.max.z = std::max(bounds.max.z, vertex.z);
    }

    return bounds;
}

void AppendPatch(
    const TriangulatedPatch& patch,
    const SurfaceType positiveTag,
    const SurfaceType negativeTag,
    Fragment& positive,
    Fragment& negative)
{
    const auto positiveBase = static_cast<std::uint32_t>(positive.vertices.size());
    const auto negativeBase = static_cast<std::uint32_t>(negative.vertices.size());

    positive.vertices.insert(positive.vertices.end(), patch.vertices.begin(), patch.vertices.end());
    negative.vertices.insert(negative.vertices.end(), patch.vertices.begin(), patch.vertices.end());

    for (std::size_t i = 0; i + 2 < patch.indices.size(); i += 3)
    {
        positive.indices.push_back(positiveBase + patch.indices[i + 0]);
        positive.indices.push_back(positiveBase + patch.indices[i + 1]);
        positive.indices.push_back(positiveBase + patch.indices[i + 2]);
        positive.triangleSurfaceTags.push_back(positiveTag);

        negative.indices.push_back(negativeBase + patch.indices[i + 2]);
        negative.indices.push_back(negativeBase + patch.indices[i + 1]);
        negative.indices.push_back(negativeBase + patch.indices[i + 0]);
        negative.triangleSurfaceTags.push_back(negativeTag);
    }
}

Fragment BuildFragmentFromTaggedMesh(const TaggedMesh& mesh, const std::uint32_t id)
{
    Fragment fragment{};
    fragment.id = id;
    fragment.vertices = mesh.vertices;
    fragment.indices = mesh.indices;
    fragment.triangleSurfaceTags = mesh.triangleSurfaceTags;
    fragment.bounds = ComputeBounds(fragment.vertices);
    return fragment;
}

} // namespace

FragmentRebuildResult RebuildFragments(
    const SliceIntersectionsResult& sliced,
    const TriangulationResult& triangulated)
{
    FragmentRebuildResult result{};
    result.positiveFragment = BuildFragmentFromTaggedMesh(sliced.positiveMesh, 1);
    result.negativeFragment = BuildFragmentFromTaggedMesh(sliced.negativeMesh, 2);

    for (const auto& patch : triangulated.cutSurfacePatches)
    {
        AppendPatch(
            patch,
            SurfaceType::CutSurface,
            SurfaceType::CutSurface,
            result.positiveFragment,
            result.negativeFragment);
    }

    for (const auto& patch : triangulated.cutCapPatches)
    {
        AppendPatch(
            patch,
            SurfaceType::CutCap,
            SurfaceType::CutCap,
            result.positiveFragment,
            result.negativeFragment);
    }

    result.positiveFragment.bounds = ComputeBounds(result.positiveFragment.vertices);
    result.negativeFragment.bounds = ComputeBounds(result.negativeFragment.vertices);

    return result;
}

void fragment_rebuild_module_anchor()
{
}

} // namespace cotrx::core
