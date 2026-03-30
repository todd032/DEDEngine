#include "core/procedural_cube_generator.hpp"

#include <algorithm>
#include <array>

namespace cotrx::core
{

namespace
{
constexpr float kMinimumHalfExtent = 1.0e-4f;

Fragment CreateCubeShell(
    const std::uint32_t fragmentId,
    const Vec3& center,
    const Vec3& halfExtent,
    const SurfaceType surfaceType)
{
    const std::array<Vec3, 8> localVertices = {
        Vec3{-halfExtent.x, -halfExtent.y, -halfExtent.z},
        Vec3{halfExtent.x, -halfExtent.y, -halfExtent.z},
        Vec3{halfExtent.x, halfExtent.y, -halfExtent.z},
        Vec3{-halfExtent.x, halfExtent.y, -halfExtent.z},
        Vec3{-halfExtent.x, -halfExtent.y, halfExtent.z},
        Vec3{halfExtent.x, -halfExtent.y, halfExtent.z},
        Vec3{halfExtent.x, halfExtent.y, halfExtent.z},
        Vec3{-halfExtent.x, halfExtent.y, halfExtent.z}};

    const std::array<std::uint32_t, 36> indices = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        0, 4, 5, 0, 5, 1,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        0, 3, 7, 0, 7, 4};

    Fragment fragment{};
    fragment.id = fragmentId;
    fragment.vertices.reserve(localVertices.size());
    for (const auto& vertex : localVertices)
    {
        fragment.vertices.push_back(center + vertex);
    }

    fragment.indices.assign(indices.begin(), indices.end());
    fragment.triangleSurfaceTags.assign(fragment.indices.size() / 3, surfaceType);
    fragment.bounds = {center - halfExtent, center + halfExtent};
    return fragment;
}
} // namespace

ProceduralCubeShellPair GenerateProceduralCubeShellPair(const ProceduralCubeGeneratorConfig& config)
{
    const Vec3 clampedOuterHalfExtent{
        std::max(config.outerHalfExtent.x, kMinimumHalfExtent),
        std::max(config.outerHalfExtent.y, kMinimumHalfExtent),
        std::max(config.outerHalfExtent.z, kMinimumHalfExtent)};

    const Vec3 clampedInnerHalfExtent{
        std::max(clampedOuterHalfExtent.x - config.skinThickness, kMinimumHalfExtent),
        std::max(clampedOuterHalfExtent.y - config.skinThickness, kMinimumHalfExtent),
        std::max(clampedOuterHalfExtent.z - config.skinThickness, kMinimumHalfExtent)};

    return {
        CreateCubeShell(config.outerFragmentId, config.center, clampedOuterHalfExtent, SurfaceType::OuterSurface),
        CreateCubeShell(config.innerFragmentId, config.center, clampedInnerHalfExtent, SurfaceType::InnerSurface)};
}

void procedural_cube_generator_module_anchor()
{
}

} // namespace cotrx::core
