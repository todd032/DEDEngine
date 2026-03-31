cmake_minimum_required(VERSION 3.24)

project(CookieOnTheRoofX VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(COTRX_BUILD_TESTS "Build simulation tests." ON)

if(NOT EMSCRIPTEN)
    set(COTRX_SDL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2")
    if(NOT EXISTS "${COTRX_SDL_ROOT}/CMakeLists.txt")
        message(FATAL_ERROR "Expected SDL2 source tree at ${COTRX_SDL_ROOT}")
    endif()

    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_STATIC_PIC ON CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

    add_subdirectory("${COTRX_SDL_ROOT}" EXCLUDE_FROM_ALL)

    function(cotrx_pick_target out_var)
        foreach(candidate IN LISTS ARGN)
            if(TARGET "${candidate}")
                set(${out_var} "${candidate}" PARENT_SCOPE)
                return()
            endif()
        endforeach()

        message(FATAL_ERROR "Unable to find any target from: ${ARGN}")
    endfunction()

    cotrx_pick_target(COTRX_SDL_TARGET
        SDL2::SDL2-static
        SDL2::SDL2
        SDL2-static
        SDL2)

    cotrx_pick_target(COTRX_SDL_MAIN_TARGET
        SDL2::SDL2main
        SDL2main)
endif()

set(COTRX_CORE_SOURCES
    src/engine/types.hpp
    src/engine/geometry.hpp
    src/engine/geometry.cpp
    src/game/simulation.hpp
    src/game/simulation.cpp
    src/core/surface_types.hpp
    src/core/surface_types.cpp
    src/core/mesh_types.hpp
    src/core/mesh_types.cpp
    src/core/fragment_mesh.hpp
    src/core/fragment_mesh.cpp
    src/core/plane_math.hpp
    src/core/plane_math.cpp
    src/core/slice_intersections.hpp
    src/core/slice_intersections.cpp
    src/core/contour_builder.hpp
    src/core/contour_builder.cpp
    src/core/region_builder.hpp
    src/core/region_builder.cpp
    src/core/triangulation.hpp
    src/core/triangulation.cpp
    src/core/fragment_rebuild.hpp
    src/core/fragment_rebuild.cpp
    src/core/procedural_cube_generator.hpp
    src/core/procedural_cube_generator.cpp
    src/validation/scenario_runner.hpp
    src/validation/scenario_runner.cpp)

add_library(cotrx_core STATIC
    ${COTRX_CORE_SOURCES})

target_include_directories(cotrx_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(cotrx_core
    PRIVATE
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)

set(COTRX_VIEWER_SOURCES
    src/viewer/main.hpp
    src/viewer/main.cpp
    src/viewer/debug_draw.hpp
    src/viewer/debug_draw.cpp
    src/viewer/imgui_panels.hpp
    src/viewer/imgui_panels.cpp)

set(COTRX_RUNTIME_SOURCES
    src/engine/gl_api.hpp
    src/engine/gl_api.cpp
    src/engine/renderer.hpp
    src/engine/renderer.cpp)

if(EMSCRIPTEN)
    add_library(cotrx_runtime STATIC ${COTRX_RUNTIME_SOURCES})
    target_compile_options(cotrx_runtime PUBLIC "SHELL:-sUSE_SDL=2")
    target_link_options(cotrx_runtime INTERFACE "SHELL:-sUSE_SDL=2")
    target_link_libraries(cotrx_runtime PUBLIC cotrx_core)
elseif(WIN32)
    add_library(cotrx_runtime STATIC ${COTRX_RUNTIME_SOURCES})
    target_link_libraries(cotrx_runtime PUBLIC cotrx_core ${COTRX_SDL_TARGET} opengl32)
elseif(ANDROID)
    add_library(cotrx_runtime STATIC ${COTRX_RUNTIME_SOURCES})
    target_link_libraries(cotrx_runtime PUBLIC cotrx_core ${COTRX_SDL_TARGET} log android EGL GLESv3)
endif()

if(EMSCRIPTEN)
    add_library(cotrx_platform STATIC
        src/platform/web/platform_config.cpp)
elseif(WIN32)
    add_library(cotrx_platform STATIC
        src/platform/windows/platform_config.cpp)
elseif(ANDROID)
    add_library(cotrx_platform STATIC
        src/platform/android/platform_config.cpp)
else()
    add_library(cotrx_platform STATIC
        src/platform/windows/platform_config.cpp)
endif()

target_link_libraries(cotrx_platform PUBLIC cotrx_runtime)

set(COTRX_ENTRY_SOURCE src/platform/shared/sdl_main.cpp)

if(ANDROID)
    add_library(main SHARED ${COTRX_ENTRY_SOURCE})
    target_link_libraries(main PRIVATE cotrx_platform)
elseif(EMSCRIPTEN)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/site")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/web/.nojekyll" "${CMAKE_BINARY_DIR}/site/.nojekyll" COPYONLY)
    set(COTRX_WEB_SHELL "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")

    add_executable(CookieOnTheRoofX ${COTRX_ENTRY_SOURCE})
    set_target_properties(CookieOnTheRoofX PROPERTIES
        OUTPUT_NAME "index"
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/site"
        LINK_DEPENDS "${COTRX_WEB_SHELL}")

    target_link_libraries(CookieOnTheRoofX PRIVATE cotrx_platform)
    target_link_options(CookieOnTheRoofX PRIVATE
        "SHELL:-sUSE_SDL=2"
        "SHELL:-sWASM=1"
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
        "SHELL:-sMIN_WEBGL_VERSION=2"
        "SHELL:-sMAX_WEBGL_VERSION=2"
        "SHELL:-sFULL_ES3=1"
        "SHELL:-sENVIRONMENT=web"
        "SHELL:--shell-file ${COTRX_WEB_SHELL}")
elseif(WIN32)
    add_executable(CookieOnTheRoofX WIN32 ${COTRX_ENTRY_SOURCE})
    set_target_properties(CookieOnTheRoofX PROPERTIES OUTPUT_NAME "Cookie On The Roof X")
    target_link_libraries(CookieOnTheRoofX PRIVATE cotrx_platform ${COTRX_SDL_MAIN_TARGET})
else()
    add_executable(CookieOnTheRoofX ${COTRX_ENTRY_SOURCE})
    set_target_properties(CookieOnTheRoofX PROPERTIES OUTPUT_NAME "CookieOnTheRoofX")
    target_link_libraries(CookieOnTheRoofX PRIVATE cotrx_platform ${COTRX_SDL_MAIN_TARGET})
endif()

if(NOT ANDROID AND NOT EMSCRIPTEN)
    add_executable(viewer
        src/viewer/viewer_main.cpp
        ${COTRX_VIEWER_SOURCES})
    target_link_libraries(viewer PRIVATE cotrx_core)

    find_package(glfw3 QUIET)
    if(TARGET glfw)
        target_link_libraries(viewer PRIVATE glfw)
    elseif(TARGET glfw3::glfw)
        target_link_libraries(viewer PRIVATE glfw3::glfw)
    endif()

    find_package(glad QUIET)
    if(TARGET glad::glad)
        target_link_libraries(viewer PRIVATE glad::glad)
    elseif(TARGET glad)
        target_link_libraries(viewer PRIVATE glad)
    endif()

    find_package(glm QUIET)
    if(TARGET glm::glm)
        target_link_libraries(viewer PRIVATE glm::glm)
    endif()

    find_package(imgui QUIET)
    if(TARGET imgui::imgui)
        target_link_libraries(viewer PRIVATE imgui::imgui)
    elseif(TARGET imgui)
        target_link_libraries(viewer PRIVATE imgui)
    endif()

    add_executable(validation_runner
        src/validation/validation_runner_main.cpp)
    target_link_libraries(validation_runner PRIVATE cotrx_core)
endif()

if(COTRX_BUILD_TESTS AND NOT ANDROID AND NOT EMSCRIPTEN)
    include(CTest)
    enable_testing()

    add_executable(cotrx_simulation_tests
        tests/simulation_tests.cpp)

    target_link_libraries(cotrx_simulation_tests PRIVATE cotrx_core)

    add_test(NAME cotrx_simulation_tests COMMAND cotrx_simulation_tests)

    add_executable(cotrx_mesh_slicing_validation_tests
        tests/mesh_slicing_validation_tests.cpp)

    target_link_libraries(cotrx_mesh_slicing_validation_tests PRIVATE cotrx_core)

    add_test(NAME cotrx_mesh_slicing_validation_tests COMMAND cotrx_mesh_slicing_validation_tests)
endif()
