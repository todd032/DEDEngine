# Fresh Machine Setup

## Goal

This file is for bringing `Cookie On The Roof X` up on a different machine with minimal guesswork.

## Repo Shape

Best case is copying or pulling this whole repo:

- the project lives at the repo root
- optional repo-local tool caches live under `.external-cache`
- web output is generated into `build/web/site`

## Windows Requirements

Install these first:

- Visual Studio 2022
- Workload: `Desktop development with C++`
- Windows 10/11 SDK

Optional but useful:

- CMake in `PATH`
- Ninja in `PATH`

## Windows Build

From the project folder:

```bat
tools\build_windows_ascii_path.bat
```

What it does:

- temporarily maps the project root to `X:`
- runs `VsDevCmd.bat`
- runs CMake configure + build
- copies the final exe to `dist/Cookie On The Roof X.exe`

Expected result:

- `dist/Cookie On The Roof X.exe`

## Web Requirements

For local web builds on Windows:

- `git`
- `python`
- `cmake`
- `ninja`

The repo-local web helper script will populate:

- `.external-cache\emsdk`

Run:

```powershell
tools\build_web.ps1
```

Expected result:

- `build\web\site\index.html`
- `build\web\site\index.js`
- `build\web\site\index.wasm`

If you only want to validate the bash path used in CI:

```bash
bash tools/build_web.sh
```

## If The Web Build Fails

Check these in order:

- `git` exists and can clone `emsdk`
- `python` exists
- `cmake` exists either in `PATH` or a repo-local cache under `.external-cache\cmake`
- `ninja` exists in `PATH`, a repo-local cache, or the Visual Studio CMake toolset
- `EMSDK` is set by the script before `cmake --preset web-emscripten` runs

Fallback options:

- install CMake locally and rerun the script
- install Ninja locally and rerun the script
- delete `.external-cache\emsdk` and let the script bootstrap it again

## GitHub Pages

The deployment workflow is:

- `.github/workflows/deploy-web.yml`

Manual repo step still required:

- GitHub repository settings -> **Pages** -> **Source** -> **GitHub Actions**

Expected Pages URL:

- `https://todd032.github.io/DEDEngine/`

## Android Requirements

Install these before trying APK builds:

- Android Studio
- Android SDK Platform 35
- Android SDK Build-Tools
- Android NDK side-by-side
- JDK 17

Then open:

- `android`

## Android Notes

Prepared already:

- Gradle project
- SDLActivity-based `MainActivity`
- manifest with landscape orientation
- GLES 3.0 required

Not verified yet on this machine:

- real `assembleDebug` APK build
