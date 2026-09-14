# AGENTS.md

Double Click Hotkey is a C++17 Windows 11 x64 utility with a portable core and Linux/Windows tests.

## Development

- Keep launch parsing and orchestration in `src/application/`; Win32 operations belong in `src/platform/windows/` behind
  `include/double_click_hotkey/platform_binding.hpp`. Core and tests must have no Win32 APIs or headers.
- Dependencies: executable → Windows adapter → core; tests → core and GoogleTest only. Keep `src/main.cpp` platform
  neutral and select the binding factory through CMake; exclude the adapter from native Linux builds.
- Ship one executable with only Windows-provided DLL dependencies. Preserve static linking, size optimizations,
  debugging information in `Debug` and `RelWithDebInfo`, and stream-free output as described in
  [release guidance](docs/release.md).
- Register new C++ files in `DOUBLE_CLICK_HOTKEY_FORMAT_FILES` in `CMakeLists.txt`, and test sources in
  `tests/CMakeLists.txt`.
- Update documentation with behavior, workflow, and build changes. Keep README focused on common tasks; put reference
  details in `docs/`, avoid duplication, and update affected links.

## Validation

- Add only portable unit tests through interfaces and fakes that run unchanged on Linux and Windows. No Win32 APIs,
  Windows headers, or platform-specific test gating. Run CTest only with native-host presets; retain cross-build
  `DISCOVERY_MODE PRE_TEST`.
- Check for build size regressions whenever changes can affect the binary (application code, build/link settings,
  dependencies, or resources). Follow [release validation](docs/release.md#validation): measure Release before and after
  with the same toolchain and configuration, report byte counts and percentage change, investigate unexpected growth,
  and fix regressions or explain necessary increases. Inspect DLL imports and ASLR/NX as part of the check.
- Manually check affected Windows behavior: F13 handling, console output/visibility, and launch commands. Report
  unavailable checks as pending; cross-compilation does not validate runtime behavior.
- Prefer `npm run build`, `npm run test`, and `npm run format:check`; apply formatting with `npm run format`. See
  [development](docs/development.md) for prerequisites, direct CMake commands, and the repository layout.
