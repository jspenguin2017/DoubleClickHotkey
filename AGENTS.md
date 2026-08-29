# AGENTS.md

Double Click Hotkey is a C++17 Windows 11 utility that turns F8 into a global double-click shortcut. CMake also exposes
the platform-independent application controller and hotkey policy as a library so they can be built and tested natively
on Linux.

## Repository Map

- `src/main.cpp` is the platform-neutral composition root. It creates the selected platform binding and runs the
  application controller.
- `include/double_click_hotkey/application.hpp` and `src/application/application.cpp` define portable application
  orchestration over the abstract platform contract.
- `include/double_click_hotkey/platform_binding.hpp` defines the input/output boundary implemented by native adapters;
  `include/double_click_hotkey/platform_factory.hpp` declares the CMake-selected binding factory.
- `include/double_click_hotkey/hotkey_action.hpp` declares the portable modifier-to-action API.
- `src/hotkey/hotkey_action.cpp` implements that API and is compiled into the `DoubleClickHotkey::core` static library.
- `src/platform/windows/` contains the Win32 binding, divided into console, control-handler, keyboard-hook, mouse, and
  single-instance components. No Win32 APIs belong in the portable application or hotkey domains.
- `tests/application/application_test.cpp` verifies orchestration through a fake binding, while
  `tests/hotkey/hotkey_action_test.cpp` covers the portable hotkey policy; `tests/CMakeLists.txt` defines the test
  executable.
- `CMakeLists.txt` defines the core library, the CMake-selected platform adapter, optional application, GoogleTest
  dependency, warnings, and C++ formatting targets.
- `CMakePresets.json` contains native Linux, Linux-to-Windows MinGW-w64, and Windows MinGW-w64 configure/build/test
  presets. Build output goes under `build/<preset>`.
- `cmake/ClangFormat.cmake` defines `format` and `format-check` when `clang-format` is available.
- `cmake/toolchains/mingw-w64-x86_64.cmake` configures the Linux-to-Windows cross-compilation toolchain.
- `README.md` is the user-facing behavior, setup, build, test, and installation guide.
- `package.json`, `.prettierrc.json`, and `.prettierignore` configure formatting for documentation and configuration
  files. Prettier does not format the C++ sources.
- `CODE_REVIEW_BUILD_SETUP.md` and `CODE_REVIEW_RUNTIME.md` are review records tied to an older repository revision;
  verify their claims against the current source before acting on them.

## Common Commands

Run commands from the repository root. Node.js 22.12 or newer is preferred; CMake requires version 3.28 or newer.

- Install the locked document-formatting dependency with `npm install`.
- Format supported documentation and configuration files with `npm run format`. Use `npm run format:nocache` when a full
  formatting pass must ignore Prettier's cache, and `npm run format:check` for a non-writing check.
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
