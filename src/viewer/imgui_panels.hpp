#pragma once

#include "viewer/main.hpp"

namespace cotrx::viewer
{

void BuildManipulatorPanel(ViewerState& state);
void BuildSelectionPanel(ViewerState& state);
void BuildRenderModePanel(ViewerState& state);
void BuildExportPanel(ViewerState& state);

void imgui_panels_module_anchor();

} // namespace cotrx::viewer
