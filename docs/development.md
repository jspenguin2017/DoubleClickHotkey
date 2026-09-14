# Development

See the [README](../README.md#build-test-and-format) for npm shortcuts and [AGENTS.md](../AGENTS.md) for contribution
constraints. Run commands from the repository root. Build hosts are Linux or Windows; the app runs only on Windows 11.

## Prerequisites

- CMake 3.28+, Ninja, and a C++17 compiler. Windows app builds require an x64 MinGW-w64 toolchain; Linux tests use the
  host compiler.
- npm shortcuts require Node.js and npm (see the [Node version requirement](../package.json)). `npm install` installs
  Prettier; install clang-format separately for C++ formatting. Direct CMake builds do not require Node.js.

On Ubuntu 24, install the C++ build tools with:

```sh
sudo apt install clang-format cmake g++ mingw-w64 ninja-build
```

The Linux cross-toolchain uses `x86_64-w64-mingw32-gcc`, `x86_64-w64-mingw32-g++`, and `x86_64-w64-mingw32-windres`.
Override `MINGW_W64_TRIPLE` during initial configuration for another prefix. On Windows, put the MinGW-w64 toolchain's
`gcc` and `g++` on PATH.

Debug presets fetch pinned GoogleTest sources from GitHub on first configuration, requiring network access. Release
presets disable tests and do not fetch GoogleTest.

## Direct CMake

Choose a preset from [CMakePresets.json](../CMakePresets.json):

| Host    | Preset                  | Builds                         |
| ------- | ----------------------- | ------------------------------ |
| Linux   | `linux-native-debug`    | Portable core and native tests |
| Linux   | `linux-mingw-debug`     | Windows app and tests          |
| Linux   | `linux-mingw-release`   | Windows app                    |
| Windows | `windows-mingw-debug`   | Windows app and native tests   |
| Windows | `windows-mingw-release` | Windows app                    |

Use the same preset for configuration and build. For a Release cross-build on Linux:

```sh
cmake --preset linux-mingw-release
cmake --build --preset linux-mingw-release
```

Output goes under `build/<preset>`. Distribute `build/<release-preset>/DoubleClickHotkey.exe`; see
[release validation](release.md#validation) for size and dependency checks.

For native tests on Linux:

```sh
cmake --preset linux-native-debug
cmake --build --preset linux-native-debug
ctest --preset linux-native-debug
```

Use `windows-mingw-debug` for native Windows tests. These are the only CTest presets; cross-built Windows tests must run
on Windows. For C++ formatting, append `--target format` or `--target format-check` to the build command; install
clang-format before configuration. Prettier formats documentation and configuration files, not C++.

## Repository layout

- `src/main.cpp` creates the CMake-selected platform binding and runs the controller. Public contracts are in
  `include/double_click_hotkey/`; portable launch parsing and orchestration live in `src/application/`.
- `src/platform/windows/` implements console/control handling, instance commands, keyboard hooks and sending, mouse
  input, and single-instance ownership.
- `tests/application/` tests the core through a fake binding. `tests/main.test.cpp` supplies a fake platform factory;
  `tests/CMakeLists.txt` builds the entrypoint under a test-only function name.
- `CMakeLists.txt` defines targets, GoogleTest, warnings, and size flags; `cmake/` holds formatting and toolchain setup.
  `package.json`, `scripts/cmake.mjs`, and `.prettier*` configure npm workflows and documentation formatting.
