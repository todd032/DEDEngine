#include "platform/shared/platform_config.hpp"

namespace cotrx
{
EngineConfig MakePlatformConfig()
{
    EngineConfig config{};
    config.windowWidth = 1360;
    config.windowHeight = 820;
    config.targetFps = 60;
    config.title = "Cookie On The Roof X";
    config.clearColor = {0.95f, 0.95f, 0.92f, 1.0f};
    config.preferGles = true;
    config.landscapeOnly = false;
    return config;
}
} // namespace cotrx
