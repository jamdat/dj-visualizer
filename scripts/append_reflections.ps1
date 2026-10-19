$path = 'G:\My Drive\AGY\.cortex\memory_logs\session_reflections.md'
$entry = @'

### [2026-10-06] DJ Visualizer: Antigravity IDE C++ Debugging via CodeLLDB, F5 Task Resolution & Portable Client Packaging
- **Friction**:
  1. Microsoft's cppvsdbg debugger failed in Antigravity IDE because it requires the proprietary closed-source ms-vscode.cpptools extension;
  2. Emptying launch.json caused VS Code's debug resolver to query active language extensions on F5, triggering golang.go which threw an error searching for the go binary;
  3. User keybinding overrides without a "when": "!inDebugMode" clause have lower specificity than default keybindings and get ignored;
  4. Standalone packaging for DirectX/C++ client distribution requires pairing d3dcompiler_47.dll, AOT .cso bytecode, editable .hlsl, and presets with a 1-click launcher.
- **Learned Rule**:
  - In Antigravity IDE / open-source VS Code forks, always use vadimcn.vscode-lldb (type: "lldb") installed via antigravity-ide.cmd --install-extension vadimcn.vscode-lldb rather than cppvsdbg for native C++ debugging on Windows.
  - Never leave launch.json with empty configurations in workspaces containing multi-language extensions (like Go/Python), as VS Code's debug engine falls back to arbitrary active providers on F5. Always define an explicit launch target or specify preLaunchTask alongside type "lldb".
  - When packaging standalone portable DirectX deliverables, bundle d3dcompiler_47.dll from the build directory or Windows SDK alongside precompiled .cso shaders, default JSON presets, and a detached start "" "App.exe" batch launcher to ensure zero-dependency client execution.
'@
Add-Content -Path $path -Value $entry -Encoding utf8
Write-Host "Appended reflection entry successfully"
