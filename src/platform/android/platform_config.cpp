#include "platform/shared/platform_config.hpp"

namespace cotrx
{
EngineConfig MakePlatformConfig()
{
    EngineConfig config{};
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.targetFps = 60;
    config.title = "DEDEngine";
    config.clearColor = {0.95f, 0.95f, 0.92f, 1.0f};
    config.preferGles = true;
    config.landscapeOnly = true;
    return config;
}
} // namespace cotrx
