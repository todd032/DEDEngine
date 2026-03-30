#include "core/surface_types.hpp"

namespace cotrx::core
{

bool IsSkinBoundary(const SurfaceType surfaceType)
{
    return (surfaceType == SurfaceType::OuterSurface) || (surfaceType == SurfaceType::CutSurface);
}

bool IsMeatBoundary(const SurfaceType surfaceType)
{
    return (surfaceType == SurfaceType::InnerSurface) || (surfaceType == SurfaceType::CutCap);
}

} // namespace cotrx::core
