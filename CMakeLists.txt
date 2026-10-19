cmake_minimum_required(VERSION 3.20)
project(DJVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set runtime output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

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

# Source files
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

    # UI
    src/ui/DjDashboard.h
    src/ui/DjDashboard.cpp

    # Third Party: Dear ImGui
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp
    third_party/imgui/backends/imgui_impl_dx11.cpp
)

# Executable definition
if(MSVC)
    add_executable(DJVisualizer WIN32 ${SOURCES})
    target_compile_options(DJVisualizer PRIVATE /W3 /permissive- /utf-8 /EHsc)
else()
    add_executable(DJVisualizer ${SOURCES})
endif()

target_compile_definitions(DJVisualizer PRIVATE IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)

# Link Libraries
target_link_libraries(DJVisualizer PRIVATE
    d3d11
    dxgi
    d3dcompiler
    dwmapi
    avrt
    winmm
    ole32
)

# Locate FXC Shader Compiler
find_program(FXC_EXECUTABLE fxc
    HINTS
    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64"
    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
    "C:/Program Files (x86)/Windows Kits/10/bin/x64"
)

# Dependency-tracked HLSL Shader Compilation (.hlsl -> .cso only when modified)
set(SHADER_NAMES
    ModularMixer
    AlivePyramid
    SynthwaveGrid
    RaymarchedCrystals
    ConcertLasers
    PostProcessGlitch
)

set(SHADER_OUTPUTS "")
set(COMMON_HLSL "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/Common.hlsl")
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/bin/src/shaders")

if(FXC_EXECUTABLE)
    foreach(SHADER_NAME ${SHADER_NAMES})
        set(HLSL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/${SHADER_NAME}.hlsl")
        set(HLSL_DST "${SHADER_OUT_DIR}/${SHADER_NAME}.hlsl")
        set(CSO_OUT "${SHADER_OUT_DIR}/${SHADER_NAME}.cso")
        add_custom_command(
            OUTPUT "${CSO_OUT}" "${HLSL_DST}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUT_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${COMMON_HLSL}" "${SHADER_OUT_DIR}/Common.hlsl"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HLSL_SRC}" "${HLSL_DST}"
            COMMAND "${FXC_EXECUTABLE}" /T ps_5_0 /E PSMain /O3 /Fo "${CSO_OUT}" /I "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders" "${HLSL_SRC}"
            DEPENDS "${HLSL_SRC}" "${COMMON_HLSL}"
            COMMENT "Precompiling ${SHADER_NAME}.hlsl -> ${SHADER_NAME}.cso (/O3)"
            VERBATIM
        )
        list(APPEND SHADER_OUTPUTS "${CSO_OUT}" "${HLSL_DST}")
    endforeach()

    add_custom_target(CompileShaders DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(DJVisualizer CompileShaders)
endif()

# Post-Build: Ensure asset/preset directories exist
add_custom_command(TARGET DJVisualizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:DJVisualizer>/src/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:DJVisualizer>/assets/textures
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:DJVisualizer>/presets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/ue5_bridge $<TARGET_FILE_DIR:DJVisualizer>/ue5_bridge
    COMMENT "Ensuring runtime directories and UE5 bridge files are up to date"
)
