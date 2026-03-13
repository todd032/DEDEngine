#pragma once

#include <functional>

#include "types.hpp"

namespace cotrx
{
void AppendMesh(MeshData& target, const MeshData& source);
MeshData CreateQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Color& color);
MeshData CreateBox(const Vec3& center, float width, float height, float depth, const Color& color);
MeshData CreateOrientedBox(const Vec3& center, const Vec3& halfX, const Vec3& halfY, const Vec3& halfZ, const Color& color);
MeshData CreateSurfaceSolid(
    int uSegments,
    int vSegments,
    const std::function<Vec3(float, float)>& topSampler,
    const std::function<Vec3(float, float)>& bottomSampler,
    const Color& color);
} // namespace cotrx
