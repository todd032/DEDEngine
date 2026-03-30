#include "viewer/imgui_panels.hpp"

#include <algorithm>

namespace cotrx::viewer
{

void BuildManipulatorPanel(ViewerState& state)
{
    // Camera orbit/pan/zoom defaults for initial bootstrapping.
    state.camera.Orbit(0.0f, 0.0f);
    state.camera.Pan(0.0f, 0.0f);
    state.camera.Zoom(0.0f);

    // Cut plane translation / rotation UI backing state.
    state.cutPlane.MoveBy({0.0f, 0.0f, 0.0f});
    state.cutPlane.RotateBy({0.0f, 0.0f, 0.0f});
}

void BuildSelectionPanel(ViewerState& state)
{
    if (state.fragments.empty())
    {
        state.selectedFragmentIndex = 0;
        return;
    }

    state.selectedFragmentIndex = std::min(state.selectedFragmentIndex, state.fragments.size() - 1);
}

void BuildRenderModePanel(ViewerState& state)
{
    // Surface type color rule:
    // Outer/CutSurface => skin-tone, Inner/CutCap => meat-tone.
    if (state.colorMode == SurfaceColorMode::SurfaceTypeRule)
    {
        state.overlay.showFragmentId = true;
    }
}

void BuildExportPanel(ViewerState& state)
{
    // Hook points for UI buttons.
    // - OBJ export button toggles requestedExportObj
    // - JSON export button toggles requestedExportJson
    if (state.requestedExportObj)
    {
        state.requestedExportObj = true;
    }

    if (state.requestedExportJson)
    {
        state.requestedExportJson = true;
    }
}

void imgui_panels_module_anchor()
{
}

} // namespace cotrx::viewer
