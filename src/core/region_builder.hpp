#pragma once

#include <cstdint>
#include <vector>

#include "core/contour_builder.hpp"

namespace cotrx::core
{

enum class RegionKind
{
    Skin,
    Meat
};

struct Region
{
    std::uint32_t regionId = 0;
    RegionKind kind = RegionKind::Skin;
    std::uint32_t outerLoopId = 0;
    std::vector<std::uint32_t> holeLoopIds;
};

struct RegionBuildResult
{
    std::vector<Region> skinRegions;
    std::vector<Region> meatRegions;
};

RegionBuildResult BuildRegions(const ContourBuildResult& contour, const SliceEpsilon& epsilon);

void region_builder_module_anchor();

} // namespace cotrx::core
