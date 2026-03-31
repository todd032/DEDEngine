#include "viewer/debug_draw.hpp"

#include <iomanip>
#include <sstream>

namespace cotrx::viewer
{

std::vector<OverlayLine> BuildOverlayLines(const ViewerState& state)
{
    std::vector<OverlayLine> lines;

    if (state.overlay.showFragmentId)
    {
        std::ostringstream oss;
        oss << "Selected Fragment: " << state.selectedFragmentIndex;
        lines.push_back({oss.str()});
    }

    if (state.overlay.showTriangleStats)
    {
        for (const auto& fragmentStats : state.stats)
        {
            std::ostringstream oss;
            oss << "Fragment " << fragmentStats.fragmentId << " | Triangles: " << fragmentStats.triangleCount
                << " | Area: " << std::fixed << std::setprecision(3) << fragmentStats.totalArea;
            lines.push_back({oss.str()});
        }
    }

    if (state.overlay.showWireframe)
    {
        lines.push_back({"Wireframe: ON"});
    }

    if (state.overlay.showNormals)
    {
        lines.push_back({"Normals: ON (if available)"});
    }

    return lines;
}

std::vector<OverlayPrimitive> BuildOverlayDebugPrimitives(const ViewerState& state)
{
    std::vector<OverlayPrimitive> primitives;
    if (!state.overlay.showFragmentId && !state.overlay.showTriangleStats)
    {
        return primitives;
    }

    // Renderer text path is still backend-specific, so emit a simple frame-like
    // primitive as a draw-call marker for overlay pass verification.
    primitives.push_back(OverlayPrimitive{8.0f, 8.0f, 260.0f, 4.0f});
    return primitives;
}

void DrawViewerOverlays(ViewerState& state)
{
    [[maybe_unused]] const auto lines = BuildOverlayLines(state);
    [[maybe_unused]] const auto primitives = BuildOverlayDebugPrimitives(state);
    state.overlayDrawCallCount += 1;
    state.overlayPrimitiveCount = primitives.size();
}

void debug_draw_module_anchor()
{
}

} // namespace cotrx::viewer
