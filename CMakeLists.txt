cmake_minimum_required(VERSION 3.20)

if(APPLE)
    project(DJVisualizer LANGUAGES CXX OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
else()
    project(DJVisualizer LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Version & GitHub Repository metadata for Auto-Updater
if(NOT DEFINED DJVISUALIZER_VERSION)
    set(DJVISUALIZER_VERSION "v1.0.2")
endif()
if(NOT DEFINED DJVISUALIZER_GITHUB_REPO)
    set(DJVISUALIZER_GITHUB_REPO "jamdat/dj-visualizer")
endif()
# macOS privacy permissions are keyed by bundle ID; override for local dev builds to keep them separate from releases
if(NOT DEFINED DJVISUALIZER_BUNDLE_ID)
    set(DJVISUALIZER_BUNDLE_ID "com.daftalive.djvisualizer")
endif()
# macOS bundle signing identity. An ad-hoc ("-") signature changes on every build, so macOS forgets the
# privacy permissions; a self-signed code signing certificate named "DJVisualizer Local Dev" in the login
# keychain keeps them across rebuilds and is picked up automatically at configure time when present
if(APPLE AND NOT DEFINED DJVISUALIZER_CODESIGN_IDENTITY)
    execute_process(COMMAND security find-identity -p codesigning
        OUTPUT_VARIABLE _djv_identities ERROR_QUIET)
    string(FIND "${_djv_identities}" "\"DJVisualizer Local Dev\"" _djv_identity_pos)
    if(_djv_identity_pos GREATER -1)
        set(DJVISUALIZER_CODESIGN_IDENTITY "DJVisualizer Local Dev")
    else()
        set(DJVISUALIZER_CODESIGN_IDENTITY "-")
    endif()
endif()
if(APPLE)
    message(STATUS "macOS code signing identity: ${DJVISUALIZER_CODESIGN_IDENTITY}")
endif()

# Set runtime output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# Fetch GLFW 3.4 for cross-platform Windowing & OpenGL 3.3 Core Contexts
include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fft
)

# Common Source files
set(SOURCES
    src/main.cpp
    src/core/Logger.h
    src/core/Logger.cpp

    # Audio
    src/audio/WasapiAudioCapture.h
    src/audio/WasapiAudioCapture.cpp
    src/audio/AudioAnalyzer.h
    src/audio/AudioAnalyzer.cpp
    src/audio/BeatDetector.h
    src/audio/BeatDetector.cpp

    # Graphics
    src/graphics/GL33Loader.h
    src/graphics/ConstantBuffers.h
    src/graphics/ShaderCompiler.h
    src/graphics/ShaderCompiler.cpp
    src/graphics/TextureManager.h
    src/graphics/TextureManager.cpp
    src/graphics/SpoutSender.h
    src/graphics/SpoutSender.cpp
    src/graphics/WindowManager.h
    src/graphics/WindowManager.cpp
    src/graphics/D3D11Renderer.h
    src/graphics/D3D11Renderer.cpp
    src/graphics/Mesh3D.h
    src/graphics/Mesh3D.cpp

    # Control
    src/control/PresetManager.h
    src/control/PresetManager.cpp
    src/control/AutoPilot.h
    src/control/AutoPilot.cpp
    src/control/MidiController.h
    src/control/MidiController.cpp
    src/control/AutoUpdater.h
    src/control/AutoUpdater.cpp

    # UI
    src/ui/DjDashboard.h
    src/ui/DjDashboard.cpp

    # Third Party: Dear ImGui (Cross-Platform GLFW + OpenGL 3.3 Core)
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

if(APPLE)
    list(APPEND SOURCES src/audio/MacAudioCapture.mm)
endif()

# Executable definition
if(MSVC)
    add_executable(DJVisualizer WIN32 ${SOURCES})
    target_compile_options(DJVisualizer PRIVATE /W3 /permissive- /utf-8 /EHsc)
elseif(APPLE)
    add_executable(DJVisualizer MACOSX_BUNDLE ${SOURCES})
    set_target_properties(DJVisualizer PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "${DJVISUALIZER_BUNDLE_ID}"
        MACOSX_BUNDLE_BUNDLE_NAME "DJVisualizer"
        MACOSX_BUNDLE_BUNDLE_VERSION "${DJVISUALIZER_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${DJVISUALIZER_VERSION}"
        MACOSX_BUNDLE_INFO_STRING "DJ Audio-Reactive Visualizer (Daft Alive Edition)"
    )
    target_compile_options(DJVisualizer PRIVATE -Wno-deprecated-declarations -fobjc-arc)
else()
    add_executable(DJVisualizer ${SOURCES})
endif()

target_compile_definitions(DJVisualizer PRIVATE
    DJVISUALIZER_VERSION="${DJVISUALIZER_VERSION}"
    DJVISUALIZER_GITHUB_REPO="${DJVISUALIZER_GITHUB_REPO}"
)

# Link Libraries
if(WIN32)
    target_link_libraries(DJVisualizer PRIVATE
        glfw
        OpenGL::GL
        dwmapi
        avrt
        winmm
        ole32
    )
elseif(APPLE)
    target_compile_definitions(DJVisualizer PRIVATE GLFW_INCLUDE_NONE)
    target_link_libraries(DJVisualizer PRIVATE
        glfw
        OpenGL::GL
        "-framework Foundation"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework AudioToolbox"
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework ScreenCaptureKit"
    )
endif()

# Post-Build: Copy Shaders, Assets, and Presets next to executable (and inside macOS .app bundle)
if(APPLE)
    add_custom_command(TARGET DJVisualizer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/src/shaders
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/assets
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/src/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/presets $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/presets
        # Resources only: non-code files in Contents/MacOS can't be sealed and break the bundle signature
        # macOS only grants mic / line-in access (and shows the permission prompt) when the bundle declares a usage string
        COMMAND plutil -replace NSMicrophoneUsageDescription -string "DJVisualizer listens to your microphone, DJ mixer, or audio interface to drive audio-reactive visuals." $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Info.plist
        # System audio (Serato, Spotify, ...) is captured with a Core Audio process tap, which needs this usage string
        COMMAND plutil -replace NSAudioCaptureUsageDescription -string "DJVisualizer listens to the audio playing on this Mac, such as Serato or Spotify, to drive audio-reactive visuals." $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Info.plist
        # Older builds wrote ImGui's layout file into the bundle; it now lives in Application Support
        COMMAND ${CMAKE_COMMAND} -E rm -f $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/imgui.ini
        # Sign the finished bundle so macOS privacy permissions (TCC) can validate it
        COMMAND codesign --force --timestamp=none --sign "${DJVISUALIZER_CODESIGN_IDENTITY}" $<TARGET_BUNDLE_DIR:DJVisualizer>
        COMMENT "Bundling HLSL shaders, assets, and presets into macOS DJVisualizer.app"
    )
else()
    add_custom_command(TARGET DJVisualizer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:DJVisualizer>/src/shaders
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:DJVisualizer>/assets
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:DJVisualizer>/presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders $<TARGET_FILE_DIR:DJVisualizer>/src/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:DJVisualizer>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/presets $<TARGET_FILE_DIR:DJVisualizer>/presets
        COMMENT "Copying HLSL shaders, assets, and presets to output directory"
    )
endif()

# The POST_BUILD copies above only run when DJVisualizer relinks, so a shader-only edit never reached
# the app. Re-sync shaders, assets, and presets (and re-sign the macOS bundle) whenever one of them changes.
file(GLOB_RECURSE DJVISUALIZER_RUNTIME_RESOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/*
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/*
)
set(DJVISUALIZER_RESOURCES_STAMP ${CMAKE_CURRENT_BINARY_DIR}/runtime_resources.stamp)
if(APPLE)
    set(_djv_resource_dir $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources)
    set(_djv_sign_command COMMAND codesign --force --timestamp=none --sign "${DJVISUALIZER_CODESIGN_IDENTITY}" $<TARGET_BUNDLE_DIR:DJVisualizer>)
else()
    set(_djv_resource_dir $<TARGET_FILE_DIR:DJVisualizer>)
    set(_djv_sign_command "")
endif()
add_custom_command(OUTPUT ${DJVISUALIZER_RESOURCES_STAMP}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders ${_djv_resource_dir}/src/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets ${_djv_resource_dir}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/presets ${_djv_resource_dir}/presets
    ${_djv_sign_command}
    COMMAND ${CMAKE_COMMAND} -E touch ${DJVISUALIZER_RESOURCES_STAMP}
    DEPENDS ${DJVISUALIZER_RUNTIME_RESOURCES}
    COMMENT "Syncing changed shaders, assets, and presets into DJVisualizer"
    VERBATIM
)
add_custom_target(DJVisualizerResources ALL DEPENDS ${DJVISUALIZER_RESOURCES_STAMP})
add_dependencies(DJVisualizerResources DJVisualizer)
