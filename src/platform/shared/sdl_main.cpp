#include <SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "engine/renderer.hpp"
#include "game/simulation.hpp"
#include "platform/shared/platform_config.hpp"

namespace
{
enum class AppMode : std::uint8_t
{
    Launcher = 0,
    CookieOnTheRoofX,
    MeshSlicePrototype,
};

AppMode ParseInitialMode(int argc, char* argv[])
{
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index] != nullptr ? argv[index] : "";
        if (argument == "--mode=cookie")
        {
            return AppMode::CookieOnTheRoofX;
        }
        if (argument == "--mode=mesh")
        {
            return AppMode::MeshSlicePrototype;
        }
        if (argument == "--mode=launcher")
        {
            return AppMode::Launcher;
        }
    }

    return AppMode::Launcher;
}

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

struct LauncherButton
{
    AppMode targetMode = AppMode::Launcher;
    std::string label;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool hovered = false;
    bool pressed = false;

    [[nodiscard]] bool Contains(float px, float py) const
    {
        return px >= x && py >= y && px <= (x + width) && py <= (y + height);
    }
};

struct App
{
    cotrx::EngineConfig config = cotrx::MakePlatformConfig();
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    cotrx::Renderer renderer;
    cotrx::Simulation simulation{1337u};
    AppMode currentMode = AppMode::Launcher;
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
    std::array<LauncherButton, 2> launcherButtons{
        LauncherButton{AppMode::CookieOnTheRoofX, "COOKIE ON THE ROOF"},
        LauncherButton{AppMode::MeshSlicePrototype, "MESH SLICE"}};
    int launcherPressedButton = -1;
    SDL_FingerID launcherPressedTouchId = 0;
    std::array<LauncherButton, 3> meshButtons{
        LauncherButton{AppMode::MeshSlicePrototype, "TOGGLE GRID"},
        LauncherButton{AppMode::MeshSlicePrototype, "TOGGLE WIREFRAME"},
        LauncherButton{AppMode::MeshSlicePrototype, "CYCLE BACKGROUND"}};
    int meshPressedButton = -1;
    SDL_FingerID meshPressedTouchId = 0;
    bool meshShowGrid = true;
    bool meshShowWireframe = false;
    int meshBackgroundPreset = 0;
    cotrx::SimulationState blankRenderState{};

    void RefreshLauncherLayout()
    {
        constexpr float buttonWidth = 340.0f;
        constexpr float buttonHeight = 54.0f;
        constexpr float buttonGap = 18.0f;
        const auto startX = (static_cast<float>(windowWidth) - buttonWidth) * 0.5f;
        const auto startY = (static_cast<float>(windowHeight) * 0.5f) - buttonHeight - (buttonGap * 0.5f);

        for (std::size_t index = 0; index < launcherButtons.size(); ++index)
        {
            auto& button = launcherButtons[index];
            button.x = startX;
            button.y = startY + static_cast<float>(index) * (buttonHeight + buttonGap);
            button.width = buttonWidth;
            button.height = buttonHeight;
        }
    }

    void RefreshMeshLayout()
    {
        constexpr float buttonWidth = 320.0f;
        constexpr float buttonHeight = 50.0f;
        constexpr float buttonGap = 14.0f;
        const auto startX = (static_cast<float>(windowWidth) - buttonWidth) * 0.5f;
        const auto startY = (static_cast<float>(windowHeight) * 0.5f) - ((buttonHeight * 1.5f) + buttonGap);

        for (std::size_t index = 0; index < meshButtons.size(); ++index)
        {
            auto& button = meshButtons[index];
            button.x = startX;
            button.y = startY + static_cast<float>(index) * (buttonHeight + buttonGap);
            button.width = buttonWidth;
            button.height = buttonHeight;
        }
    }

    [[nodiscard]] int HitTestLauncherButton(float x, float y) const
    {
        for (std::size_t index = 0; index < launcherButtons.size(); ++index)
        {
            if (launcherButtons[index].Contains(x, y))
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    [[nodiscard]] int HitTestMeshButton(float x, float y) const
    {
        for (std::size_t index = 0; index < meshButtons.size(); ++index)
        {
            if (meshButtons[index].Contains(x, y))
            {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    void SetLauncherHover(float x, float y)
    {
        const auto hoveredIndex = HitTestLauncherButton(x, y);
        for (std::size_t index = 0; index < launcherButtons.size(); ++index)
        {
            launcherButtons[index].hovered = static_cast<int>(index) == hoveredIndex;
        }
    }

    void SetLauncherPressed(int pressedButton)
    {
        launcherPressedButton = pressedButton;
        for (std::size_t index = 0; index < launcherButtons.size(); ++index)
        {
            launcherButtons[index].pressed = static_cast<int>(index) == pressedButton;
        }
    }

    void ClearLauncherPointerState()
    {
        SetLauncherPressed(-1);
        launcherPressedTouchId = 0;
    }

    void SetMeshHover(float x, float y)
    {
        const auto hoveredIndex = HitTestMeshButton(x, y);
        for (std::size_t index = 0; index < meshButtons.size(); ++index)
        {
            meshButtons[index].hovered = static_cast<int>(index) == hoveredIndex;
        }
    }

    void SetMeshPressed(int pressedButton)
    {
        meshPressedButton = pressedButton;
        for (std::size_t index = 0; index < meshButtons.size(); ++index)
        {
            meshButtons[index].pressed = static_cast<int>(index) == pressedButton;
        }
    }

    void ClearMeshPointerState()
    {
        SetMeshPressed(-1);
        meshPressedTouchId = 0;
    }

    void ApplyMeshButtonAction(int buttonIndex)
    {
        if (buttonIndex == 0)
        {
            meshShowGrid = !meshShowGrid;
        }
        else if (buttonIndex == 1)
        {
            meshShowWireframe = !meshShowWireframe;
        }
        else if (buttonIndex == 2)
        {
            meshBackgroundPreset = (meshBackgroundPreset + 1) % 3;
        }
    }

    [[nodiscard]] const cotrx::SimulationState& RenderStateForCurrentMode() const
    {
        if (currentMode == AppMode::CookieOnTheRoofX)
        {
            return simulation.State();
        }
        return blankRenderState;
    }

    [[nodiscard]] cotrx::Color ClearColorForCurrentMode() const
    {
        if (currentMode == AppMode::Launcher)
        {
            return {0.08f, 0.09f, 0.11f, 1.0f};
        }

        if (currentMode == AppMode::MeshSlicePrototype)
        {
            switch (meshBackgroundPreset)
            {
            case 1:
                return {0.10f, 0.13f, 0.18f, 1.0f};
            case 2:
                return {0.14f, 0.10f, 0.16f, 1.0f};
            default:
                return {0.09f, 0.10f, 0.12f, 1.0f};
            }
        }

        return config.clearColor;
    }

    void RebuildSceneForCurrentMode()
    {
        renderer.RebuildScene(RenderStateForCurrentMode());
        lastSceneRevision = simulation.SceneRevision();
    }

    void SetMode(AppMode mode)
    {
        currentMode = mode;
        mouseOrbiting = false;
        mousePressedAction = cotrx::UiAction::None;
        touchPressedAction = cotrx::UiAction::None;
        touchPressedId = 0;
        orbitFingerId = 0;
        lastPinchDistance = 0.0f;
        touches = {};
        simulation.SetPressedAction(cotrx::UiAction::None);
        ClearLauncherPointerState();
        ClearMeshPointerState();
        RebuildSceneForCurrentMode();
    }

    [[nodiscard]] cotrx::UiOverlayState BuildUiStateForCurrentMode() const
    {
        cotrx::UiOverlayState uiState{};
        uiState.versionLabel = simulation.State().versionLabel;

        if (currentMode == AppMode::Launcher)
        {
            uiState.hudLines = {
                "PROTOTYPE SELECTOR",
                "프로토타입 선택",
                "COOKIE ON THE ROOF / MESH SLICE"};
            uiState.footer = "PROTOTYPE LAUNCHER";

            uiState.buttons.reserve(launcherButtons.size());
            for (const auto& button : launcherButtons)
            {
                cotrx::ButtonState uiButton{};
                uiButton.label = button.label;
                uiButton.x = button.x;
                uiButton.y = button.y;
                uiButton.width = button.width;
                uiButton.height = button.height;
                uiButton.hovered = button.hovered;
                uiButton.pressed = button.pressed;
                uiState.buttons.push_back(uiButton);
            }
            return uiState;
        }

        if (currentMode == AppMode::MeshSlicePrototype)
        {
            meshButtons[0].label = std::string("GRID: ") + (meshShowGrid ? "ON" : "OFF");
            meshButtons[1].label = std::string("WIREFRAME: ") + (meshShowWireframe ? "ON" : "OFF");
            meshButtons[2].label = std::string("BACKGROUND: ") + std::to_string(meshBackgroundPreset + 1);
            uiState.hudLines = {
                "MESH SLICE PROTOTYPE",
                std::string("GRID ") + (meshShowGrid ? "ON" : "OFF"),
                std::string("WIREFRAME ") + (meshShowWireframe ? "ON" : "OFF"),
                "MANUAL VISUAL CHECK READY",
                "PRESS ESC TO RETURN"};
            uiState.footer = "MESH SLICE PROTOTYPE";

            uiState.buttons.reserve(meshButtons.size());
            for (const auto& button : meshButtons)
            {
                cotrx::ButtonState uiButton{};
                uiButton.label = button.label;
                uiButton.x = button.x;
                uiButton.y = button.y;
                uiButton.width = button.width;
                uiButton.height = button.height;
                uiButton.hovered = button.hovered;
                uiButton.pressed = button.pressed;
                uiState.buttons.push_back(uiButton);
            }
            return uiState;
        }

        const auto& state = simulation.State();
        uiState.buttons = state.buttons;
        uiState.hudLines = state.hudLines;
        uiState.footer = "COOKIE ON THE ROOF";
        return uiState;
    }

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
        RefreshLauncherLayout();
        RefreshMeshLayout();
        blankRenderState = simulation.State();
        blankRenderState.meshes.clear();
        blankRenderState.sceneObjects.clear();
        blankRenderState.buttons.clear();
        blankRenderState.hudLines.clear();
        blankRenderState.versionLabel = simulation.State().versionLabel;
        RebuildSceneForCurrentMode();
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

        if (currentMode == AppMode::CookieOnTheRoofX && simulation.SceneRevision() != lastSceneRevision)
        {
            renderer.RebuildScene(simulation.State());
            lastSceneRevision = simulation.SceneRevision();
        }

        const auto uiState = BuildUiStateForCurrentMode();
        renderer.Render(RenderStateForCurrentMode(), uiState, ClearColorForCurrentMode(), windowWidth, windowHeight, drawableWidth, drawableHeight);
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
        RefreshLauncherLayout();
        RefreshMeshLayout();
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

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE && currentMode != AppMode::Launcher)
            {
                SetMode(AppMode::Launcher);
            }
            break;

        case SDL_MOUSEMOTION:
        {
            const auto x = static_cast<float>(event.motion.x);
            const auto y = static_cast<float>(event.motion.y);
            if (currentMode == AppMode::Launcher)
            {
                SetLauncherHover(x, y);
                break;
            }
            if (currentMode == AppMode::MeshSlicePrototype)
            {
                SetMeshHover(x, y);
                break;
            }
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
                if (currentMode == AppMode::Launcher)
                {
                    SetLauncherPressed(HitTestLauncherButton(lastMouseX, lastMouseY));
                    break;
                }
                if (currentMode == AppMode::MeshSlicePrototype)
                {
                    SetMeshPressed(HitTestMeshButton(lastMouseX, lastMouseY));
                    break;
                }
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
                if (currentMode == AppMode::Launcher)
                {
                    const auto releasedButton = HitTestLauncherButton(releaseX, releaseY);
                    if (launcherPressedButton >= 0 && launcherPressedButton == releasedButton)
                    {
                        SetMode(launcherButtons[static_cast<std::size_t>(launcherPressedButton)].targetMode);
                    }
                    ClearLauncherPointerState();
                    SetLauncherHover(releaseX, releaseY);
                    break;
                }
                if (currentMode == AppMode::MeshSlicePrototype)
                {
                    const auto releasedButton = HitTestMeshButton(releaseX, releaseY);
                    if (meshPressedButton >= 0 && meshPressedButton == releasedButton)
                    {
                        ApplyMeshButtonAction(meshPressedButton);
                    }
                    ClearMeshPointerState();
                    SetMeshHover(releaseX, releaseY);
                    break;
                }
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
            if (currentMode == AppMode::CookieOnTheRoofX)
            {
                simulation.ZoomFromInput(-event.wheel.preciseY * 0.55f);
            }
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
            if (currentMode == AppMode::Launcher)
            {
                const auto pressedButton = HitTestLauncherButton(touch->x, touch->y);
                SetLauncherPressed(pressedButton);
                launcherPressedTouchId = pressedButton >= 0 ? event.tfinger.fingerId : 0;
                SetLauncherHover(touch->x, touch->y);
                break;
            }
            if (currentMode == AppMode::MeshSlicePrototype)
            {
                const auto pressedButton = HitTestMeshButton(touch->x, touch->y);
                SetMeshPressed(pressedButton);
                meshPressedTouchId = pressedButton >= 0 ? event.tfinger.fingerId : 0;
                SetMeshHover(touch->x, touch->y);
                break;
            }
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
            if (currentMode == AppMode::Launcher)
            {
                SetLauncherHover(touch->x, touch->y);
                if (launcherPressedTouchId == event.tfinger.fingerId)
                {
                    SetLauncherPressed(HitTestLauncherButton(touch->x, touch->y));
                }
                break;
            }
            if (currentMode == AppMode::MeshSlicePrototype)
            {
                SetMeshHover(touch->x, touch->y);
                if (meshPressedTouchId == event.tfinger.fingerId)
                {
                    SetMeshPressed(HitTestMeshButton(touch->x, touch->y));
                }
                break;
            }
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
            if (currentMode == AppMode::Launcher)
            {
                const auto releasedButton = HitTestLauncherButton(releaseX, releaseY);
                if (launcherPressedTouchId == event.tfinger.fingerId && launcherPressedButton >= 0 && launcherPressedButton == releasedButton)
                {
                    SetMode(launcherButtons[static_cast<std::size_t>(launcherPressedButton)].targetMode);
                }
                if (launcherPressedTouchId == event.tfinger.fingerId)
                {
                    ClearLauncherPointerState();
                }
                SetLauncherHover(releaseX, releaseY);
                ReleaseTouch(touches, event.tfinger.fingerId);
                break;
            }
            if (currentMode == AppMode::MeshSlicePrototype)
            {
                const auto releasedButton = HitTestMeshButton(releaseX, releaseY);
                if (meshPressedTouchId == event.tfinger.fingerId && meshPressedButton >= 0 && meshPressedButton == releasedButton)
                {
                    ApplyMeshButtonAction(meshPressedButton);
                }
                if (meshPressedTouchId == event.tfinger.fingerId)
                {
                    ClearMeshPointerState();
                }
                SetMeshHover(releaseX, releaseY);
                ReleaseTouch(touches, event.tfinger.fingerId);
                break;
            }

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
    const auto initialMode = ParseInitialMode(argc, argv);

#ifdef __EMSCRIPTEN__
    auto* app = new App();
    app->currentMode = initialMode;
    if (!app->Initialize())
    {
        delete app;
        return 1;
    }

    emscripten_set_main_loop_arg(&TickApp, app, 0, 1);
    return 0;
#else
    App app;
    app.currentMode = initialMode;
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
