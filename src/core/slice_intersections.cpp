#include "core/slice_intersections.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace cotrx::core
{

namespace
{
struct ClassifiedVertex
{
    std::uint32_t index = 0;
    Vec3 point{};
    float distance = 0.0f;
    HalfSpace side = HalfSpace::OnPlane;
};

HalfSpace ClassifyDistance(const float distance, const SliceEpsilon& epsilon)
{
    if (distance > epsilon.distance)
    {
        return HalfSpace::Positive;
    }

    if (distance < -epsilon.distance)
    {
        return HalfSpace::Negative;
    }

    return HalfSpace::OnPlane;
}

Vec3 IntersectEdgeWithPlane(
    const ClassifiedVertex& a,
    const ClassifiedVertex& b,
    const SliceEpsilon& epsilon)
{
    const auto denominator = a.distance - b.distance;
    if (std::abs(denominator) <= epsilon.distance)
    {
        return (a.point + b.point) * 0.5f;
    }

    const auto t = Clamp(a.distance / denominator, 0.0f, 1.0f);
    return Lerp(a.point, b.point, t);
}

SegmentEdgeKey BuildEdgeKey(const std::uint32_t a, const std::uint32_t b)
{
    if (a < b)
    {
        return {a, b};
    }

    return {b, a};
}

void AppendTriangulatedPolygon(
    const std::vector<Vec3>& polygon,
    const SurfaceType tag,
    TaggedMesh& mesh,
    const SliceEpsilon& epsilon)
{
    if (polygon.size() < 3)
    {
        return;
    }

    const auto baseIndex = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.insert(mesh.vertices.end(), polygon.begin(), polygon.end());

    for (std::size_t i = 1; i + 1 < polygon.size(); ++i)
    {
        const auto edgeA = polygon[i] - polygon[0];
        const auto edgeB = polygon[i + 1] - polygon[0];
        if (LengthSquared(Cross(edgeA, edgeB)) <= epsilon.segmentLength * epsilon.segmentLength)
        {
            continue;
        }

        mesh.indices.push_back(baseIndex);
        mesh.indices.push_back(baseIndex + static_cast<std::uint32_t>(i));
        mesh.indices.push_back(baseIndex + static_cast<std::uint32_t>(i + 1));
        mesh.triangleSurfaceTags.push_back(tag);
    }
}

std::vector<Vec3> ClipPolygonAgainstSide(
    const std::array<ClassifiedVertex, 3>& triangle,
    const bool keepPositive,
    const SliceEpsilon& epsilon)
{
    std::vector<Vec3> output;
    output.reserve(4);

    for (std::size_t i = 0; i < triangle.size(); ++i)
    {
        const auto& current = triangle[i];
        const auto& next = triangle[(i + 1) % triangle.size()];

        const auto currentInside = keepPositive
            ? current.distance >= -epsilon.distance
            : current.distance <= epsilon.distance;
        const auto nextInside = keepPositive
            ? next.distance >= -epsilon.distance
            : next.distance <= epsilon.distance;

        if (currentInside && nextInside)
        {
            output.push_back(next.point);
            continue;
        }

        if (currentInside && !nextInside)
        {
            output.push_back(IntersectEdgeWithPlane(current, next, epsilon));
            continue;
        }

        if (!currentInside && nextInside)
        {
            output.push_back(IntersectEdgeWithPlane(current, next, epsilon));
            output.push_back(next.point);
        }
    }

    return output;
}

void CollectCutSegment(
    const std::array<ClassifiedVertex, 3>& triangle,
    const std::uint32_t triangleId,
    const SurfaceType surfaceType,
    const SliceEpsilon& epsilon,
    std::vector<CutSegment>& outSegments)
{
    struct EdgeHit
    {
        Vec3 point{};
        SegmentEdgeKey key{};
    };

    std::vector<EdgeHit> hits;
    hits.reserve(3);

    constexpr std::array<std::pair<int, int>, 3> edges = {{{0, 1}, {1, 2}, {2, 0}}};
    for (const auto [a, b] : edges)
    {
        const auto& va = triangle[a];
        const auto& vb = triangle[b];

        if (va.side == HalfSpace::OnPlane && vb.side == HalfSpace::OnPlane)
        {
            continue;
        }

        const auto oppositeSigns = (va.distance > epsilon.distance && vb.distance < -epsilon.distance) ||
            (va.distance < -epsilon.distance && vb.distance > epsilon.distance);

        if (!oppositeSigns && va.side != HalfSpace::OnPlane && vb.side != HalfSpace::OnPlane)
        {
            continue;
        }

        const auto point = IntersectEdgeWithPlane(va, vb, epsilon);
        auto duplicate = false;
        for (const auto& existing : hits)
        {
            if (LengthSquared(existing.point - point) <= epsilon.mergeRadius * epsilon.mergeRadius)
            {
                duplicate = true;
                break;
            }
        }

        if (!duplicate)
        {
            hits.push_back({point, BuildEdgeKey(va.index, vb.index)});
        }
    }

    if (hits.size() != 2)
    {
        return;
    }

    if (LengthSquared(hits[0].point - hits[1].point) < epsilon.segmentLength * epsilon.segmentLength)
    {
        return;
    }

    if ((hits[1].point.x < hits[0].point.x) ||
        (hits[1].point.x == hits[0].point.x && hits[1].point.y < hits[0].point.y) ||
        (hits[1].point.x == hits[0].point.x && hits[1].point.y == hits[0].point.y && hits[1].point.z < hits[0].point.z))
    {
        std::swap(hits[0], hits[1]);
    }

    outSegments.push_back({
        triangleId,
        hits[0].point,
        hits[1].point,
        hits[0].key,
        hits[1].key,
        IsSkinBoundary(surfaceType) ? CutBoundaryKind::Skin : CutBoundaryKind::Meat});
}

} // namespace

SliceIntersectionsResult SliceIntersections(
    const std::vector<Vec3>& vertices,
    const std::vector<TriangleView>& triangles,
    const Plane& plane,
    const SliceEpsilon& epsilon)
{
    SliceIntersectionsResult result;

    for (std::uint32_t triangleId = 0; triangleId < triangles.size(); ++triangleId)
    {
        const auto& triangle = triangles[triangleId];
        const std::array<std::uint32_t, 3> indices = {triangle.index0, triangle.index1, triangle.index2};

        std::array<ClassifiedVertex, 3> classified{};
        for (std::size_t i = 0; i < 3; ++i)
        {
            classified[i].index = indices[i];
            classified[i].point = vertices[indices[i]];
            classified[i].distance = SignedDistanceToPlane(plane, classified[i].point);
            classified[i].side = ClassifyDistance(classified[i].distance, epsilon);
        }

        const auto positivePolygon = ClipPolygonAgainstSide(classified, true, epsilon);
        const auto negativePolygon = ClipPolygonAgainstSide(classified, false, epsilon);

        AppendTriangulatedPolygon(positivePolygon, triangle.surfaceType, result.positiveMesh, epsilon);
        AppendTriangulatedPolygon(negativePolygon, triangle.surfaceType, result.negativeMesh, epsilon);
        CollectCutSegment(classified, triangleId, triangle.surfaceType, epsilon, result.cutSegments);
    }

    std::sort(result.cutSegments.begin(), result.cutSegments.end(), [](const CutSegment& a, const CutSegment& b) {
        if (a.triangleId != b.triangleId)
        {
            return a.triangleId < b.triangleId;
        }

        if (a.edge0.low != b.edge0.low)
        {
            return a.edge0.low < b.edge0.low;
        }

        if (a.edge0.high != b.edge0.high)
        {
            return a.edge0.high < b.edge0.high;
        }

        if (a.edge1.low != b.edge1.low)
        {
            return a.edge1.low < b.edge1.low;
        }

        return a.edge1.high < b.edge1.high;
    });

    return result;
}

void slice_intersections_module_anchor()
{
}

} // namespace cotrx::core
