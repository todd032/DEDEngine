#pragma once

#include <cstdint>
#include <vector>

#include "core/surface_types.hpp"
#include "engine/types.hpp"

namespace cotrx::core
{

struct Bounds3
{
    Vec3 min{};
    Vec3 max{};
};

struct Fragment
{
    std::uint32_t id = 0;
    std::vector<Vec3> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<SurfaceType> triangleSurfaceTags;
    Bounds3 bounds{};
};

} // namespace cotrx::core
