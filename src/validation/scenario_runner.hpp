#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/plane_math.hpp"
#include "core/procedural_cube_generator.hpp"

namespace cotrx::validation
{

enum class CutPlanePreset
{
    InitialVisibility,
    VerticalHalfCut,
    VerticalThenHorizontalCut,
    ProgressiveSkinRemoval,
    ShallowCutNoMeatExposure
};

struct CutPlaneScenarioInput
{
    std::string name;
    std::vector<core::Plane> planes;
};

struct ScenarioCheckInput
{
    CutPlanePreset preset = CutPlanePreset::InitialVisibility;
    bool innerShellIntersected = false;
    float cutCapArea = 0.0f;
};

struct ScenarioCheckResult
{
    bool passed = true;
    std::string message;
};

struct ScenarioExecutionOptions
{
    bool dumpObj = true;
    std::optional<std::string> screenshotPath;
    core::ProceduralCubeGeneratorConfig generatorConfig{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.2f, 11u, 22u};
};

struct ScenarioExecutionResult
{
    std::string scenarioName;
    std::string outputDirectory;
    std::string jsonPath;
    std::optional<std::string> objPath;
    bool success = false;
    std::vector<std::string> assertionMessages;
};

CutPlaneScenarioInput BuildCutPlaneScenario(
    CutPlanePreset preset,
    const core::ProceduralCubeGeneratorConfig& generatorConfig);

ScenarioCheckResult RunScenarioChecks(const ScenarioCheckInput& input);

ScenarioExecutionResult RunScenarioA(const ScenarioExecutionOptions& options = {});
ScenarioExecutionResult RunScenarioB(const ScenarioExecutionOptions& options = {});
ScenarioExecutionResult RunScenarioC(const ScenarioExecutionOptions& options = {});
ScenarioExecutionResult RunScenarioD(const ScenarioExecutionOptions& options = {});
ScenarioExecutionResult RunScenarioE(const ScenarioExecutionOptions& options = {});

std::vector<ScenarioExecutionResult> RunAllScenarios(const ScenarioExecutionOptions& options = {});

void scenario_runner_module_anchor();

} // namespace cotrx::validation
