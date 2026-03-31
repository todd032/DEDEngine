#pragma once

#include <string>
#include <vector>

#include "viewer/main.hpp"

namespace cotrx::viewer
{

struct OverlayLine
{
    std::string text;
};

struct OverlayPrimitive
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

[[nodiscard]] std::vector<OverlayLine> BuildOverlayLines(const ViewerState& state);
[[nodiscard]] std::vector<OverlayPrimitive> BuildOverlayDebugPrimitives(const ViewerState& state);
void DrawViewerOverlays(ViewerState& state);

void debug_draw_module_anchor();

} // namespace cotrx::viewer
