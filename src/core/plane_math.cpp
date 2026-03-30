#include "core/plane_math.hpp"

#include <cmath>

namespace cotrx::core
{

namespace
{
constexpr float kMinNormalLength = 1.0e-8f;
}

Plane NormalizePlane(const Plane& plane)
{
    const auto length = Length(plane.normal);
    if (length <= kMinNormalLength)
    {
        return {{0.0f, 1.0f, 0.0f}, 0.0f};
    }

    return {plane.normal / length, plane.distance / length};
}

float SignedDistanceToPlane(const Plane& plane, const Vec3& point)
{
    const auto normalized = NormalizePlane(plane);
    return Dot(normalized.normal, point) + normalized.distance;
}

PlaneBasis BuildStablePlaneBasis(const Plane& plane, const Vec3& originHint)
{
    const auto normalizedPlane = NormalizePlane(plane);
    const auto n = normalizedPlane.normal;

    Vec3 seedAxis = {1.0f, 0.0f, 0.0f};
    if (std::abs(Dot(seedAxis, n)) > 0.9f)
    {
        seedAxis = {0.0f, 0.0f, 1.0f};
    }

    const auto tangent = Normalize(Cross(seedAxis, n));
    const auto bitangent = Normalize(Cross(n, tangent));

    return {
        originHint,
        tangent,
        bitangent,
        n};
}

Vec2 ProjectPointToPlane(const PlaneBasis& basis, const Vec3& point)
{
    const auto delta = point - basis.origin;
    return {Dot(delta, basis.tangent), Dot(delta, basis.bitangent)};
}

Vec3 UnprojectPointFromPlane(const PlaneBasis& basis, const Vec2& point)
{
    return basis.origin + (basis.tangent * point.x) + (basis.bitangent * point.y);
}

void plane_math_module_anchor()
{
}

} // namespace cotrx::core
