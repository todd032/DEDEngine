#include <cmath>
#include <iostream>
#include <string>

#include "core/procedural_cube_generator.hpp"
#include "core/surface_types.hpp"
#include "validation/scenario_runner.hpp"

namespace
{
bool Check(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}

bool NearlyEqual(float left, float right, float epsilon = 1.0e-4f)
{
    return std::abs(left - right) <= epsilon;
}
} // namespace

int main()
{
    using namespace cotrx;

    bool success = true;

    const core::ProceduralCubeGeneratorConfig cubeConfig{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.2f, 11u, 22u};
    const auto shellPair = core::GenerateProceduralCubeShellPair(cubeConfig);

    success &= Check(shellPair.outerShell.id == 11u, "Outer shell fragment ID mismatch.");
    success &= Check(shellPair.innerShell.id == 22u, "Inner shell fragment ID mismatch.");
    success &= Check(!shellPair.outerShell.vertices.empty(), "Outer shell must have vertices.");
    success &= Check(!shellPair.innerShell.vertices.empty(), "Inner shell must have vertices.");
    success &= Check(shellPair.outerShell.vertices.data() != shellPair.innerShell.vertices.data(), "Outer/inner shells must be independent.");
    success &= Check(shellPair.outerShell.triangleSurfaceTags.front() == core::SurfaceType::OuterSurface, "Outer shell should be tagged as OuterSurface.");
    success &= Check(shellPair.innerShell.triangleSurfaceTags.front() == core::SurfaceType::InnerSurface, "Inner shell should be tagged as InnerSurface.");

    success &= Check(NearlyEqual(shellPair.outerShell.bounds.max.x, 1.0f), "Outer shell extent should match generator half extent.");
    success &= Check(NearlyEqual(shellPair.innerShell.bounds.max.x, 0.8f), "Inner shell should apply skin thickness inset.");

    const auto initialVisibility = validation::BuildCutPlaneScenario(validation::CutPlanePreset::InitialVisibility, cubeConfig);
    const auto verticalHalfCut = validation::BuildCutPlaneScenario(validation::CutPlanePreset::VerticalHalfCut, cubeConfig);
    const auto verticalThenHorizontalCut = validation::BuildCutPlaneScenario(validation::CutPlanePreset::VerticalThenHorizontalCut, cubeConfig);
    const auto progressiveSkinRemoval = validation::BuildCutPlaneScenario(validation::CutPlanePreset::ProgressiveSkinRemoval, cubeConfig);
    const auto shallowNoMeat = validation::BuildCutPlaneScenario(validation::CutPlanePreset::ShallowCutNoMeatExposure, cubeConfig);

    success &= Check(
        initialVisibility.name == "InitialVisibility" && initialVisibility.planes.empty(),
        "[Scenario: InitialVisibility][PlaneStep: none] preset should provide zero planes.");
    success &= Check(
        verticalHalfCut.name == "VerticalHalfCut" && verticalHalfCut.planes.size() == 1u,
        "[Scenario: VerticalHalfCut][PlaneStep: 1] preset should provide one plane.");
    success &= Check(
        verticalThenHorizontalCut.name == "VerticalThenHorizontalCut" && verticalThenHorizontalCut.planes.size() == 2u,
        "[Scenario: VerticalThenHorizontalCut][PlaneStep: 1..2] preset should provide two planes.");
    success &= Check(
        progressiveSkinRemoval.name == "ProgressiveSkinRemoval" && progressiveSkinRemoval.planes.size() == 4u,
        "[Scenario: ProgressiveSkinRemoval][PlaneStep: 1..4] preset should provide four planes.");
    success &= Check(
        shallowNoMeat.name == "ShallowCutNoMeatExposure" && shallowNoMeat.planes.size() == 1u,
        "[Scenario: ShallowCutNoMeatExposure][PlaneStep: 1] preset should provide one plane.");

    const auto shallowPass = validation::RunScenarioChecks({validation::CutPlanePreset::ShallowCutNoMeatExposure, false, 0.0f});
    const auto shallowFail = validation::RunScenarioChecks({validation::CutPlanePreset::ShallowCutNoMeatExposure, false, 0.1f});

    success &= Check(
        shallowPass.passed,
        "[Scenario: ShallowCutNoMeatExposure][PlaneStep: 1] check should pass when CutCap is zero without inner-shell intersection.");
    success &= Check(
        !shallowFail.passed,
        "[Scenario: ShallowCutNoMeatExposure][PlaneStep: 1] check should fail when CutCap is non-zero without inner-shell intersection.");

    if (!success)
    {
        return 1;
    }

    std::cout << "Mesh slicing validation tests passed.\n";
    return 0;
}
