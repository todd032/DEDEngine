#pragma once

namespace cotrx::core
{
enum class SurfaceType
{
    OuterSurface,
    InnerSurface,
    CutSurface,
    CutCap
};

bool IsSkinBoundary(SurfaceType surfaceType);
bool IsMeatBoundary(SurfaceType surfaceType);

} // namespace cotrx::core
