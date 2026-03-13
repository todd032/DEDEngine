#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "game/simulation.hpp"

namespace
{
bool NearlyEqual(float left, float right, float epsilon = 1.0e-3f)
{
    return std::abs(left - right) <= epsilon;
}

bool Check(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}
} // namespace

int main()
{
    using namespace cotrx;

    auto success = true;

    const std::vector<RoofDefinition> roofs = {
        RoofDefinition::CreateFlat(3.4f, 10.0f, 10.0f, 1.0f),
        RoofDefinition{RoofShapeKind::Gable, 3.4f, 10.0f, 10.0f, 3.2f, 0.9f, 0.0f, 0.0f, 0.0f},
        RoofDefinition{RoofShapeKind::Dome, 3.4f, 10.0f, 10.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        RoofDefinition{RoofShapeKind::Pyramid, 3.4f, 10.0f, 10.0f, 3.5f, 0.0f, 0.0f, 0.0f, 0.0f},
        RoofDefinition{RoofShapeKind::Wave, 3.4f, 10.0f, 10.0f, 3.0f, 0.0f, 1.7f, 1.3f, 1.1f},
        RoofDefinition{RoofShapeKind::Shed, 3.4f, 10.0f, 10.0f, 3.1f, 0.0f, 0.0f, 0.0f, 0.0f},
    };

    for (const auto& roof : roofs)
    {
        const auto mesh = roof.BuildMesh(32, {1.0f, 0.5f, 0.2f, 1.0f});
        success &= Check(mesh.bounds.minimum.x >= -5.001f, "Roof extends beyond negative X envelope.");
        success &= Check(mesh.bounds.maximum.x <= 5.001f, "Roof extends beyond positive X envelope.");
        success &= Check(mesh.bounds.minimum.z >= -5.001f, "Roof extends beyond negative Z envelope.");
        success &= Check(mesh.bounds.maximum.z <= 5.001f, "Roof extends beyond positive Z envelope.");
        success &= Check(mesh.bounds.minimum.y >= 3.399f || roof.shapeKind == RoofShapeKind::Flat, "Roof base sinks below building top.");
        success &= Check(mesh.bounds.maximum.y <= (roof.baseY + 4.001f), "Roof exceeds max roof height envelope.");
    }

    const RoofDefinition waveRoof{RoofShapeKind::Wave, 3.4f, 10.0f, 10.0f, 3.0f, 0.0f, 1.8f, 1.4f, 0.6f};
    for (int ix = -5; ix <= 5; ++ix)
    {
        for (int iz = -5; iz <= 5; ++iz)
        {
            const auto sample = waveRoof.Sample(static_cast<float>(ix), static_cast<float>(iz));
            const auto length = Length(sample.normal);
            success &= Check(NearlyEqual(length, 1.0f, 0.01f), "Wave roof normal is not normalized.");
        }
    }

    Simulation simulation(42u);
    for (const auto& cookie : simulation.State().cookies)
    {
        success &= Check(cookie.width >= 1.0f && cookie.width <= 2.0f, "Cookie width escaped allowed range.");
        success &= Check(cookie.depth >= 1.0f && cookie.depth <= 2.0f, "Cookie depth escaped allowed range.");
        success &= Check(cookie.height >= 0.2f && cookie.height <= 0.3f, "Cookie height escaped allowed range.");
    }

    RoofDefinition gableRoof{RoofShapeKind::Gable, 3.4f, 10.0f, 10.0f, 3.2f, 1.0f, 0.0f, 0.0f, 0.0f};
    simulation.SetRoof(gableRoof);
    for (const auto& cookie : simulation.State().cookies)
    {
        const auto surfacePoint = cookie.center - (cookie.yAxis * ((cookie.height * 0.5f) + 0.02f));
        success &= Check(std::abs(surfacePoint.x) >= ((gableRoof.ridgeGap * 0.5f) - 1.0e-3f), "Gable cookie intrudes into ridge clear zone.");
    }

    if (!success)
    {
        return 1;
    }

    std::cout << "Simulation tests passed.\n";
    return 0;
}
