# AGENTS.md

Double Click Hotkey is a C++17 Windows 11 utility that turns F13 into a global double-click shortcut. CMake also exposes
the platform-independent application controller as a library so it can be built and tested natively on Linux.

## Repository Map

- `src/main.cpp` is the platform-neutral composition root. It creates the selected platform binding and runs the
  application controller.
- `include/double_click_hotkey/application.hpp`, `include/double_click_hotkey/launch_command.hpp`, and their sources
  under `src/application/` define portable launch parsing and application orchestration over the abstract platform
  contract.
- `include/double_click_hotkey/platform_binding.hpp` defines the input/output boundary implemented by native adapters;
  `include/double_click_hotkey/platform_factory.hpp` declares the CMake-selected binding factory.
- `src/platform/windows/` contains the Win32 binding, divided into console, control-handler, instance-command,
  keyboard-hook, keyboard-sender, mouse, and single-instance components. No Win32 APIs belong in the portable
  application domain.
- `tests/application/` verifies launch parsing and orchestration through a fake binding; `tests/CMakeLists.txt` defines
  the test executable.
- `CMakeLists.txt` defines the core library, the CMake-selected platform adapter, optional application, GoogleTest
  dependency, warnings, and C++ formatting targets.
- `CMakePresets.json` contains native Linux, Linux-to-Windows MinGW-w64, and Windows MinGW-w64 configure/build/test
  presets. Build output goes under `build/<preset>`.
- `cmake/ClangFormat.cmake` defines `format` and `format-check` when `clang-format` is available.
- `cmake/toolchains/mingw-w64-x86_64.cmake` configures the Linux-to-Windows cross-compilation toolchain.
- `README.md` is the user-facing behavior, setup, build, test, and installation guide.
- `package.json`, `.prettierrc.json`, and `.prettierignore` configure formatting for documentation and configuration
  files. Prettier does not format the C++ sources.

## Testing Requirements

- Add only cross-platform unit tests that compile and run in every supported native test environment, including Linux
  and Windows.
- Test platform-independent behavior through portable interfaces and fakes. Do not add unit tests that depend directly
  on Win32 APIs, Windows headers, or platform-specific test gating.

## Common Commands

Run commands from the repository root. Node.js 24.19.0 or newer and CMake 3.28 or newer are required.

- Install the locked document-formatting dependency with `npm install`.
- Format supported documentation, configuration files, and C++ sources with `npm run format`; use `npm run format:check`
  for a non-writing check.
- Configure, build, and test the portable core natively on Linux:

  ```sh
  cmake --preset linux-native-debug
  cmake --build --preset linux-native-debug
  ctest --preset linux-native-debug
  ```

- After configuring a preset on a machine with `clang-format`, format or check the C++ files with:

  ```sh
  cmake --build --preset linux-native-debug --target format
  cmake --build --preset linux-native-debug --target format-check
  ```

  Substitute another configured build preset when appropriate. Reconfigure if `clang-format` was installed after the
  initial configure step.

- Cross-compile a debug Windows build from Linux with:

  ```sh
  cmake --preset linux-mingw-debug
  cmake --build --preset linux-mingw-debug
  ```

  The resulting tests are Windows executables and are not run by a Linux CTest preset.

- Build the distributable Windows executable from Linux with:

  ```sh
  cmake --preset linux-mingw-release
  cmake --build --preset linux-mingw-release
  ```

- On Windows with MinGW-w64, use `windows-mingw-debug` for configure/build/test and `windows-mingw-release` for a
  release build. For example:

  ```sh
  cmake --preset windows-mingw-debug
  cmake --build --preset windows-mingw-debug
  ctest --preset windows-mingw-debug
  ```

The first debug configuration downloads the pinned GoogleTest source and therefore needs network access.
