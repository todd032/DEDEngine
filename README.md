# Cookie On The Roof X

## Continuation Docs

- `HANDOFF_2026-03-13.md`: current state and key assumptions
- `FRESH_MACHINE_SETUP.md`: machine setup notes for Windows and web builds
- `NEXT_STEPS.md`: recommended follow-up work order

`Cookie On The Roof X` is an SDL2-based C++20 roof-cookie placement simulation that now targets:

- Windows via SDL2 + desktop OpenGL 3.3 core
- Web via Emscripten + WebAssembly + WebGL2
- Android via SDL2 + GLES3 through the Gradle project in `android`

The old WPF `Cookie On The Roof` project remains only as a behavior reference.

## Layout

- `src/engine`: math, geometry, GL loader, renderer
- `src/game`: roof generation, cookie placement, simulation state
- `src/platform`: SDL entry point and per-platform defaults
- `tests`: headless simulation checks for geometry and placement rules
- `tools`: local build and toolchain helper scripts
- `web`: custom Emscripten shell and Pages support files
- `codex/cloud`: Codex Cloud setup and maintenance scripts for the web-build trial
- `android`: Android Studio project for debug APK builds
- `third_party/SDL2`: vendored SDL2 source tree used by Windows and Android builds

## Features

- Roof families: `Flat`, `Gable`, `Dome`, `Pyramid`, `Wave`, `Shed`
- Roof footprint fixed to `10m x 10m`, max roof height `4m`
- Cookie sizes constrained to `1m~2m` width/depth and `0.2m~0.3m` height
- Dense partition-based cookie coverage
- Gable ridge exclusion so cookies stay on sloped roof faces only
- Orbit camera with `10deg~60deg` pitch limits
- Mouse drag + wheel orbit/zoom
- Touch orbit + pinch zoom on GLES-style targets

## Web Build

Primary local entry point:

```powershell
tools\build_web.ps1
```

This script:

- clones `emsdk` into `.external-cache\emsdk` if it is missing
- activates the current `EMSDK_VERSION` or `latest`
- expects `cmake` plus `ninja` to exist either in `PATH`, a repo-local cache, or the Visual Studio CMake toolset
- configures with `cmake --preset web-emscripten`
- builds with `cmake --build --preset web-release`

The Linux/bash equivalent used by CI and Codex Cloud is:

```bash
bash tools/build_web.sh
```

Expected web output:

- `build/web/site/index.html`
- `build/web/site/index.js`
- `build/web/site/index.wasm`

### Local Web Preview

Serve `build/web/site` from a static server. Example:

```powershell
cd build\web\site
python -m http.server 8000
```

Then open `http://localhost:8000/`.

### GitHub Pages

The repo is configured for GitHub Actions-based Pages deployment through:

- `.github/workflows/deploy-web.yml`

Target URL:

- `https://todd032.github.io/DEDEngine/`

Important manual repo setting:

- In GitHub repository settings, set **Pages -> Build and deployment -> Source** to **GitHub Actions**

The workflow uploads `build/web/site` as the Pages artifact, so local build artifacts do not need to be committed.

## Windows Build

Primary helper script:

```bat
tools\build_windows_ascii_path.bat
```

The helper script assumes either:

- repo-local CMake at `.external-cache\cmake\...\bin\cmake.exe`
- or `cmake` available in `PATH`

Manual configure/build:

```powershell
cmake -S . -B build/windows-vs2022 -G "Visual Studio 17 2022" -A x64 -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
cmake --build build/windows-vs2022 --config Release
```

Expected packaged executable:

- `dist/Cookie On The Roof X.exe`

## Android Build

Open `android` in Android Studio, install the requested SDK/NDK/JDK components, and build the `app` debug variant.

Current assumptions:

- `minSdkVersion 26`
- `targetSdkVersion 35`
- landscape only
- GLES 3.0 required
- `MainActivity` inherits from SDLActivity and loads the native `main` library

## Codex Cloud Trial

Codex Cloud support for the web build is documented in:

- `codex/cloud/README.md`

Recommended environment shape:

- base image: `universal`
- setup script: `bash codex/cloud/setup-web-build.sh`
- maintenance script: `bash codex/cloud/maintenance-web-build.sh`
- agent internet access: `Off`

This keeps production deployment on GitHub Actions while letting Codex Cloud validate the same `bash tools/build_web.sh` path.

## Notes

- Windows and Android still share the SDL runtime path; the web target uses the same input handling with an Emscripten main loop.
- Web builds intentionally target WebGL2 only. There is no WebGL1 fallback in this first version.
- Roof randomization and cookie redistribution use deterministic seeds so issues are reproducible.
- In environments where MSVC try-compile linking struggles with Unicode paths, the Windows helper script uses a temporary `X:` mapping to keep the build path ASCII-only.
