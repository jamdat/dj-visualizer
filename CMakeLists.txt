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
    set(DJVISUALIZER_VERSION "v1.0.1")
endif()
if(NOT DEFINED DJVISUALIZER_GITHUB_REPO)
    set(DJVISUALIZER_GITHUB_REPO "jamdat/dj-visualizer")
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
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.daftalive.djvisualizer"
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
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/src/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/presets $<TARGET_BUNDLE_CONTENT_DIR:DJVisualizer>/Resources/presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders $<TARGET_FILE_DIR:DJVisualizer>/src/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/presets $<TARGET_FILE_DIR:DJVisualizer>/presets
        COMMENT "Bundling HLSL shaders and presets into macOS DJVisualizer.app"
    )
else()
    add_custom_command(TARGET DJVisualizer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:DJVisualizer>/src/shaders
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:DJVisualizer>/assets/textures
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:DJVisualizer>/presets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders $<TARGET_FILE_DIR:DJVisualizer>/src/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/presets $<TARGET_FILE_DIR:DJVisualizer>/presets
        COMMENT "Copying HLSL shaders and presets to output directory"
    )
endif()
