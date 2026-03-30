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

[[nodiscard]] std::vector<OverlayLine> BuildOverlayLines(const ViewerState& state);
void DrawViewerOverlays(const ViewerState& state);

void debug_draw_module_anchor();

} // namespace cotrx::viewer
