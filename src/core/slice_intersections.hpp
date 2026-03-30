#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "core/plane_math.hpp"
#include "core/surface_types.hpp"
#include "engine/types.hpp"

namespace cotrx::core
{

enum class HalfSpace
{
    Positive,
    Negative,
    OnPlane
};

enum class CutBoundaryKind
{
    Skin,
    Meat
};

struct SliceEpsilon
{
    float distance = 1.0e-6f;
    float segmentLength = 1.0e-7f;
    float mergeRadius = 5.0e-7f;
};

struct TriangleView
{
    std::uint32_t index0 = 0;
    std::uint32_t index1 = 0;
    std::uint32_t index2 = 0;
    SurfaceType surfaceType = SurfaceType::OuterSurface;
};

struct SegmentEdgeKey
{
    std::uint32_t low = 0;
    std::uint32_t high = 0;
};

struct CutSegment
{
    std::uint32_t triangleId = 0;
    Vec3 point0{};
    Vec3 point1{};
    SegmentEdgeKey edge0{};
    SegmentEdgeKey edge1{};
    CutBoundaryKind boundaryKind = CutBoundaryKind::Skin;
};

struct TaggedMesh
{
    std::vector<Vec3> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<SurfaceType> triangleSurfaceTags;
};

struct SliceIntersectionsResult
{
    TaggedMesh positiveMesh;
    TaggedMesh negativeMesh;
    std::vector<CutSegment> cutSegments;
};

SliceIntersectionsResult SliceIntersections(
    const std::vector<Vec3>& vertices,
    const std::vector<TriangleView>& triangles,
    const Plane& plane,
    const SliceEpsilon& epsilon);

void slice_intersections_module_anchor();

} // namespace cotrx::core
