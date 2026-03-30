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

void DrawViewerOverlays(const ViewerState& state)
{
    [[maybe_unused]] const auto lines = BuildOverlayLines(state);
    // Actual text rasterization is renderer-backend specific and intentionally
    // left as an integration point.
}

void debug_draw_module_anchor()
{
}

} // namespace cotrx::viewer
