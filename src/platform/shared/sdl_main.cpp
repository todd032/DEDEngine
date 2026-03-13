#include <SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "engine/renderer.hpp"
#include "game/simulation.hpp"
#include "platform/shared/platform_config.hpp"

namespace
{
struct TouchPoint
{
    SDL_FingerID id = 0;
    float x = 0.0f;
    float y = 0.0f;
    bool active = false;
};

TouchPoint* FindTouch(std::array<TouchPoint, 4>& touches, SDL_FingerID id)
{
    for (auto& touch : touches)
    {
        if (touch.active && touch.id == id)
        {
            return &touch;
        }
    }

    return nullptr;
}

TouchPoint* AcquireTouch(std::array<TouchPoint, 4>& touches, SDL_FingerID id)
{
    if (auto* existing = FindTouch(touches, id))
    {
        return existing;
    }

    for (auto& touch : touches)
    {
        if (!touch.active)
        {
            touch.active = true;
            touch.id = id;
            touch.x = 0.0f;
            touch.y = 0.0f;
            return &touch;
        }
    }

    return nullptr;
}

void ReleaseTouch(std::array<TouchPoint, 4>& touches, SDL_FingerID id)
{
    if (auto* touch = FindTouch(touches, id))
    {
        touch->active = false;
        touch->id = 0;
        touch->x = 0.0f;
        touch->y = 0.0f;
    }
}

int CountActiveTouches(const std::array<TouchPoint, 4>& touches)
{
    auto count = 0;
    for (const auto& touch : touches)
    {
        if (touch.active)
        {
            ++count;
        }
    }

    return count;
}

float DistanceBetweenFirstTwoTouches(const std::array<TouchPoint, 4>& touches)
{
    const TouchPoint* first = nullptr;
    const TouchPoint* second = nullptr;
    for (const auto& touch : touches)
    {
        if (!touch.active)
        {
            continue;
        }

        if (first == nullptr)
        {
            first = &touch;
        }
        else
        {
            second = &touch;
            break;
        }
    }

    if (first == nullptr || second == nullptr)
    {
        return 0.0f;
    }

    const auto dx = second->x - first->x;
    const auto dy = second->y - first->y;
    return std::sqrt((dx * dx) + (dy * dy));
}

float TouchToPixels(float value, int size)
{
    return value * static_cast<float>(size);
}

struct App
{
    cotrx::EngineConfig config = cotrx::MakePlatformConfig();
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    cotrx::Renderer renderer;
    cotrx::Simulation simulation{1337u};
    std::uint64_t lastSceneRevision = 0;
    int windowWidth = 0;
    int windowHeight = 0;
    int drawableWidth = 0;
    int drawableHeight = 0;
    bool sdlInitialized = false;
    bool rendererInitialized = false;
    bool running = false;
    bool mouseOrbiting = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
    cotrx::UiAction mousePressedAction = cotrx::UiAction::None;
    cotrx::UiAction touchPressedAction = cotrx::UiAction::None;
    SDL_FingerID touchPressedId = 0;
    SDL_FingerID orbitFingerId = 0;
    float lastPinchDistance = 0.0f;
    std::array<TouchPoint, 4> touches{};

    bool Initialize()
    {
#ifdef __ANDROID__
        SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
        {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }
        sdlInitialized = true;

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

        if (config.preferGles)
        {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        }
        else
        {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        }

        const auto windowFlags = static_cast<Uint32>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        window = SDL_CreateWindow(
            config.title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            config.windowWidth,
            config.windowHeight,
            windowFlags);

        if (window == nullptr)
        {
            SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
            Shutdown();
            return false;
        }

        glContext = SDL_GL_CreateContext(window);
        if (glContext == nullptr)
        {
            SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
            Shutdown();
            return false;
        }

        SDL_GL_SetSwapInterval(1);

        if (!renderer.Initialize(config.preferGles))
        {
            SDL_Log("Renderer initialization failed.");
            Shutdown();
            return false;
        }
        rendererInitialized = true;

        RefreshViewportSize();
        renderer.RebuildScene(simulation.State());
        lastSceneRevision = simulation.SceneRevision();
        running = true;
        return true;
    }

    bool Tick()
    {
        SDL_Event event{};
        while (SDL_PollEvent(&event) == 1)
        {
            HandleEvent(event);
        }

        if (!running)
        {
            return false;
        }

        if (simulation.SceneRevision() != lastSceneRevision)
        {
            renderer.RebuildScene(simulation.State());
            lastSceneRevision = simulation.SceneRevision();
        }

        renderer.Render(simulation.State(), config.clearColor, windowWidth, windowHeight, drawableWidth, drawableHeight);
        SDL_GL_SwapWindow(window);
        return running;
    }

    void Shutdown()
    {
        running = false;

        if (rendererInitialized)
        {
            renderer.Shutdown();
            rendererInitialized = false;
        }

        if (glContext != nullptr)
        {
            SDL_GL_DeleteContext(glContext);
            glContext = nullptr;
        }

        if (window != nullptr)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        if (sdlInitialized)
        {
            SDL_Quit();
            sdlInitialized = false;
        }
    }

    void RefreshViewportSize()
    {
        if (window == nullptr)
        {
            return;
        }

        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
        simulation.SetViewportSize(windowWidth, windowHeight);
    }

    void HandleEvent(const SDL_Event& event)
    {
        switch (event.type)
        {
        case SDL_QUIT:
            running = false;
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                RefreshViewportSize();
            }
            break;

        case SDL_MOUSEMOTION:
        {
            const auto x = static_cast<float>(event.motion.x);
            const auto y = static_cast<float>(event.motion.y);
            simulation.UpdateHover(x, y);
            if (mouseOrbiting)
            {
                simulation.OrbitFromPixels(x - lastMouseX, y - lastMouseY);
            }
            lastMouseX = x;
            lastMouseY = y;
            break;
        }

        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                lastMouseX = static_cast<float>(event.button.x);
                lastMouseY = static_cast<float>(event.button.y);
                mousePressedAction = simulation.HitTestButton(lastMouseX, lastMouseY);
                simulation.SetPressedAction(mousePressedAction);
                mouseOrbiting = mousePressedAction == cotrx::UiAction::None;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                const auto releaseX = static_cast<float>(event.button.x);
                const auto releaseY = static_cast<float>(event.button.y);
                if (mousePressedAction != cotrx::UiAction::None && simulation.HitTestButton(releaseX, releaseY) == mousePressedAction)
                {
                    simulation.QueueAction(mousePressedAction);
                }

                simulation.SetPressedAction(cotrx::UiAction::None);
                simulation.UpdateHover(releaseX, releaseY);
                mousePressedAction = cotrx::UiAction::None;
                mouseOrbiting = false;
            }
            break;

        case SDL_MOUSEWHEEL:
            simulation.ZoomFromInput(-event.wheel.preciseY * 0.55f);
            break;

        case SDL_FINGERDOWN:
        {
            auto* touch = AcquireTouch(touches, event.tfinger.fingerId);
            if (touch == nullptr)
            {
                break;
            }

            touch->x = TouchToPixels(event.tfinger.x, windowWidth);
            touch->y = TouchToPixels(event.tfinger.y, windowHeight);
            const auto activeCount = CountActiveTouches(touches);

            if (activeCount == 1)
            {
                touchPressedAction = simulation.HitTestButton(touch->x, touch->y);
                touchPressedId = touchPressedAction == cotrx::UiAction::None ? 0 : event.tfinger.fingerId;
                simulation.SetPressedAction(touchPressedAction);
                if (touchPressedAction == cotrx::UiAction::None)
                {
                    orbitFingerId = event.tfinger.fingerId;
                }
            }
            else if (activeCount >= 2)
            {
                orbitFingerId = 0;
                simulation.SetPressedAction(cotrx::UiAction::None);
                touchPressedAction = cotrx::UiAction::None;
                touchPressedId = 0;
                lastPinchDistance = DistanceBetweenFirstTwoTouches(touches);
            }
            break;
        }

        case SDL_FINGERMOTION:
        {
            auto* touch = FindTouch(touches, event.tfinger.fingerId);
            if (touch == nullptr)
            {
                break;
            }

            const auto previousX = touch->x;
            const auto previousY = touch->y;
            touch->x = TouchToPixels(event.tfinger.x, windowWidth);
            touch->y = TouchToPixels(event.tfinger.y, windowHeight);
            const auto activeCount = CountActiveTouches(touches);

            if (activeCount >= 2)
            {
                const auto pinchDistance = DistanceBetweenFirstTwoTouches(touches);
                if (lastPinchDistance > 0.0f)
                {
                    simulation.ZoomFromInput(-(pinchDistance - lastPinchDistance) * 0.01f);
                }
                lastPinchDistance = pinchDistance;
            }
            else if (activeCount == 1 && orbitFingerId == event.tfinger.fingerId && touchPressedAction == cotrx::UiAction::None)
            {
                simulation.OrbitFromPixels(touch->x - previousX, touch->y - previousY);
            }
            break;
        }

        case SDL_FINGERUP:
        {
            auto* touch = FindTouch(touches, event.tfinger.fingerId);
            const auto releaseX = touch != nullptr ? touch->x : TouchToPixels(event.tfinger.x, windowWidth);
            const auto releaseY = touch != nullptr ? touch->y : TouchToPixels(event.tfinger.y, windowHeight);

            if (touchPressedId == event.tfinger.fingerId && touchPressedAction != cotrx::UiAction::None)
            {
                if (simulation.HitTestButton(releaseX, releaseY) == touchPressedAction)
                {
                    simulation.QueueAction(touchPressedAction);
                }

                touchPressedAction = cotrx::UiAction::None;
                touchPressedId = 0;
                simulation.SetPressedAction(cotrx::UiAction::None);
            }

            if (orbitFingerId == event.tfinger.fingerId)
            {
                orbitFingerId = 0;
            }

            ReleaseTouch(touches, event.tfinger.fingerId);
            const auto activeCount = CountActiveTouches(touches);
            if (activeCount < 2)
            {
                lastPinchDistance = 0.0f;
            }
            break;
        }

        default:
            break;
        }
    }
};

#ifdef __EMSCRIPTEN__
void TickApp(void* userData)
{
    auto* app = static_cast<App*>(userData);
    if (app->Tick())
    {
        return;
    }

    app->Shutdown();
    delete app;
    emscripten_cancel_main_loop();
}
#endif
} // namespace

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

#ifdef __EMSCRIPTEN__
    auto* app = new App();
    if (!app->Initialize())
    {
        delete app;
        return 1;
    }

    emscripten_set_main_loop_arg(&TickApp, app, 0, 1);
    return 0;
#else
    App app;
    if (!app.Initialize())
    {
        return 1;
    }

    while (app.Tick())
    {
    }

    app.Shutdown();
    return 0;
#endif
}
