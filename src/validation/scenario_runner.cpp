#include "validation/scenario_runner.hpp"

#include <algorithm>
#include <cmath>

namespace cotrx::validation
{

namespace
{
constexpr float kAreaEpsilon = 1.0e-5f;
}

CutPlaneScenarioInput BuildCutPlaneScenario(
    const CutPlanePreset preset,
    const core::ProceduralCubeGeneratorConfig& generatorConfig)
{
    const auto halfExtentX = std::max(generatorConfig.outerHalfExtent.x, 1.0e-4f);
    const auto insetHalfExtentX = std::max(halfExtentX - generatorConfig.skinThickness, 1.0e-4f);

    switch (preset)
    {
    case CutPlanePreset::ExactHalfCut:
        return {
            "ExactHalfCut",
            {{{1.0f, 0.0f, 0.0f}, -generatorConfig.center.x}}};
    case CutPlanePreset::ShallowCut:
        return {
            "ShallowCut",
            {{{1.0f, 0.0f, 0.0f}, -(generatorConfig.center.x + ((halfExtentX + insetHalfExtentX) * 0.5f))}}};
    case CutPlanePreset::RepeatedTrimming:
        return {
            "RepeatedTrimming",
            {
                {{1.0f, 0.0f, 0.0f}, -generatorConfig.center.x},
                {{0.0f, 1.0f, 0.0f}, -(generatorConfig.center.y + (halfExtentX * 0.2f))},
                {{0.0f, 0.0f, 1.0f}, -(generatorConfig.center.z - (halfExtentX * 0.15f))},
            }};
    }

    return {};
}

ScenarioCheckResult RunScenarioChecks(const ScenarioCheckInput& input)
{
    if (input.preset == CutPlanePreset::ShallowCut && !input.innerShellIntersected)
    {
        if (std::abs(input.cutCapArea) > kAreaEpsilon)
        {
            return {
                false,
                "ShallowCut without inner-shell intersection must keep CutCap area at zero."};
        }
    }

    return {true, {}};
}

void scenario_runner_module_anchor()
{
}

} // namespace cotrx::validation
