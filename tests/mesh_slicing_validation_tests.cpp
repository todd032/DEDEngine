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

    const auto exactHalfCut = validation::BuildCutPlaneScenario(validation::CutPlanePreset::ExactHalfCut, cubeConfig);
    const auto shallowCut = validation::BuildCutPlaneScenario(validation::CutPlanePreset::ShallowCut, cubeConfig);
    const auto repeatedTrimming = validation::BuildCutPlaneScenario(validation::CutPlanePreset::RepeatedTrimming, cubeConfig);

    success &= Check(exactHalfCut.name == "ExactHalfCut" && exactHalfCut.planes.size() == 1u, "ExactHalfCut preset should provide one plane.");
    success &= Check(shallowCut.name == "ShallowCut" && shallowCut.planes.size() == 1u, "ShallowCut preset should provide one plane.");
    success &= Check(repeatedTrimming.name == "RepeatedTrimming" && repeatedTrimming.planes.size() == 3u, "RepeatedTrimming preset should provide three planes.");

    const auto shallowPass = validation::RunScenarioChecks({validation::CutPlanePreset::ShallowCut, false, 0.0f});
    const auto shallowFail = validation::RunScenarioChecks({validation::CutPlanePreset::ShallowCut, false, 0.1f});

    success &= Check(shallowPass.passed, "ShallowCut check should pass when CutCap area is zero and inner shell is not intersected.");
    success &= Check(!shallowFail.passed, "ShallowCut check should fail when CutCap area is non-zero without inner-shell intersection.");

    if (!success)
    {
        return 1;
    }

    std::cout << "Mesh slicing validation tests passed.\n";
    return 0;
}
