# Next Steps

## Recommended Order

1. Install the missing local web toolchain pieces (`cmake`, `ninja`, and repo-local `emsdk`).
2. Run `tools/build_web.ps1`.
3. Serve `build/web/site` and confirm:
   - the scene renders
   - roof randomization works
   - cookie redistribution works
   - mouse orbit + wheel zoom work
   - touch orbit + pinch zoom work
4. Push `main` and confirm the GitHub Pages workflow succeeds.
5. Switch GitHub Pages source to `GitHub Actions` if the repo has not been configured yet.
6. After web is stable, return to Windows rebuild validation.
7. Then continue with Android toolchain setup and APK validation.

## Highest Value Follow-Up Work

### 1. Web validation and polish
- test desktop browsers and mobile browsers separately
- verify canvas resize behavior under orientation changes
- pin a known-good `EMSDK_VERSION` after the first successful release build
- consider a lightweight loading progress UI if the current shell feels too bare

### 2. Android validation
- confirm Gradle sync
- confirm native CMake build from Android Studio
- confirm debug APK launches
- confirm SDLActivity loads the `main` library correctly

### 3. Build portability
- add an optional repo-local Ninja bootstrap if developer machines keep missing it
- decide whether repo-local CMake should be restored under `.external-cache`
- reduce the number of manual repo settings left outside version control

### 4. Packaging
- add an icon
- add version stamping
- tighten the `dist` story for Windows
- later add signed Android release flow if needed

## Known Gotchas

- Web builds are WebGL2 only.
- GitHub Pages requires static assets to stay on relative paths under `/DEDEngine/`.
- GitHub Pages source still has to be switched to `GitHub Actions` in the repo settings UI.
- Android has not yet been tested end-to-end in this workspace.

## Key Files To Read Before Editing

- `README.md`
- `src/platform/shared/sdl_main.cpp`
- `CMakeLists.txt`
- `web/shell.html`
- `.github/workflows/deploy-web.yml`
