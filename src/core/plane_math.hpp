#pragma once

#include "engine/types.hpp"

namespace cotrx::core
{

struct Plane
{
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;
};

struct PlaneBasis
{
    Vec3 origin{};
    Vec3 tangent{};
    Vec3 bitangent{};
    Vec3 normal{};
};

Plane NormalizePlane(const Plane& plane);
float SignedDistanceToPlane(const Plane& plane, const Vec3& point);
PlaneBasis BuildStablePlaneBasis(const Plane& plane, const Vec3& originHint = {});
Vec2 ProjectPointToPlane(const PlaneBasis& basis, const Vec3& point);
Vec3 UnprojectPointFromPlane(const PlaneBasis& basis, const Vec2& point);

void plane_math_module_anchor();

} // namespace cotrx::core
