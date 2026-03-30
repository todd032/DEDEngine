#pragma once

#include <string>
#include <vector>

#include "core/plane_math.hpp"
#include "core/procedural_cube_generator.hpp"

namespace cotrx::validation
{

enum class CutPlanePreset
{
    ExactHalfCut,
    ShallowCut,
    RepeatedTrimming
};

struct CutPlaneScenarioInput
{
    std::string name;
    std::vector<core::Plane> planes;
};

struct ScenarioCheckInput
{
    CutPlanePreset preset = CutPlanePreset::ExactHalfCut;
    bool innerShellIntersected = false;
    float cutCapArea = 0.0f;
};

struct ScenarioCheckResult
{
    bool passed = true;
    std::string message;
};

CutPlaneScenarioInput BuildCutPlaneScenario(
    CutPlanePreset preset,
    const core::ProceduralCubeGeneratorConfig& generatorConfig);

ScenarioCheckResult RunScenarioChecks(const ScenarioCheckInput& input);

void scenario_runner_module_anchor();

} // namespace cotrx::validation
