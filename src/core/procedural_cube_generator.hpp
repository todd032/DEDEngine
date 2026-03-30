#pragma once

#include <cstdint>

#include "core/mesh_types.hpp"

namespace cotrx::core
{

struct ProceduralCubeGeneratorConfig
{
    Vec3 center{0.0f, 0.0f, 0.0f};
    Vec3 outerHalfExtent{0.5f, 0.5f, 0.5f};
    float skinThickness = 0.1f;
    std::uint32_t outerFragmentId = 1;
    std::uint32_t innerFragmentId = 2;
};

struct ProceduralCubeShellPair
{
    Fragment outerShell;
    Fragment innerShell;
};

ProceduralCubeShellPair GenerateProceduralCubeShellPair(const ProceduralCubeGeneratorConfig& config);

void procedural_cube_generator_module_anchor();

} // namespace cotrx::core
