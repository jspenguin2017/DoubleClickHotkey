# Double Click Hotkey

Double Click Hotkey is a small Windows utility that turns <kbd>F8</kbd> into a global double-click shortcut.

## Behavior

Press <kbd>F8</kbd> to double-click. The shortcut does not activate while <kbd>Ctrl</kbd>, <kbd>Shift</kbd>,
<kbd>Alt</kbd>, or either <kbd>Win</kbd> key is pressed.

Press <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>Alt</kbd> + <kbd>F8</kbd> to toggle the console window. Other
applications do not receive <kbd>F8</kbd> when either shortcut is activated.

Only one copy of the application can run at a time.

## Platform support

The application is a Windows program. It can be built with MinGW-w64 on Windows or cross-compiled from Linux, but the
resulting executable only runs on Windows.

A native Linux build compiles the platform-independent core and its tests without creating the Windows application. This
gives Linux development tools and CI a native target in addition to the full MinGW-w64 cross-build.

## Requirements

All build environments need:

- CMake 3.25 or newer
- Ninja
- A compiler with C++17 support

Windows builds additionally need a 64-bit MinGW-w64 toolchain with `gcc` and `g++` available on `PATH`.

Linux development uses:

- `g++` for the native core and test build
- `x86_64-w64-mingw32-gcc`, `x86_64-w64-mingw32-g++`, and `x86_64-w64-mingw32-windres` for the Windows cross-build

On Debian or Ubuntu, the C++ build tools can be installed with:

```sh
sudo apt install cmake ninja-build g++ mingw-w64 clang-format
```

GoogleTest does not need a separate installation. Debug configurations download the pinned GoogleTest dependency the
first time CMake configures the project, so that first configuration needs network access.

Document formatting needs Node.js 20 or newer and npm. Node.js 22.12 or newer is recommended for development.
`clang-format` is only required for the C++ formatting targets and is not required to compile the project.

## Build on Windows

Run these commands from a terminal where MinGW-w64's `gcc` and `g++`, CMake, and Ninja are on `PATH`:

```sh
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug
```

To make a release build:

```sh
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
```

The release executable is `build/windows-mingw-release/DoubleClickHotkey.exe`.

## Build on Linux

Build and run the platform-independent tests with the native Linux compiler:

```sh
cmake --preset linux-native-debug
cmake --build --preset linux-native-debug
ctest --preset linux-native-debug
```

Cross-compile the Windows application and tests with MinGW-w64:

```sh
cmake --preset linux-mingw-debug
cmake --build --preset linux-mingw-debug
```

The cross-compiled tests are Windows executables and are not run by a Linux CTest preset. Use the native Linux preset
for normal test runs, or run the cross-compiled test executable on Windows or through a separately configured Windows
compatibility environment.

To make the distributable release executable:

```sh
cmake --preset linux-mingw-release
cmake --build --preset linux-mingw-release
```

The release executable is `build/linux-mingw-release/DoubleClickHotkey.exe`. Its MinGW C++ runtime is linked statically,
so the executable does not need companion MinGW runtime DLLs.

The cross-compilation toolchain defaults to the `x86_64-w64-mingw32` target triple. Override `MINGW_W64_TRIPLE` during
the initial configure step if the installed toolchain uses a different prefix.

## Tests

Tests use GoogleTest 1.17.0, pinned to a specific upstream revision and archive checksum. Test sources live in `tests/`
and link against the platform-independent `DoubleClickHotkey::core` CMake target.

Add future production source files to `double_click_hotkey_core` in `CMakeLists.txt`, or to the `double_click_hotkey`
executable when they directly depend on Windows APIs. This keeps core behavior testable on both platforms.

## Formatting

Install the locked document formatter once:

```sh
npm install
```

Prettier formats supported documents and configuration files; it does not format the C++ source:

```sh
npm run format
npm run format:check
```

After configuring any CMake preset on a machine with `clang-format`, format or check all C++ source and test files with:

```sh
cmake --build --preset linux-native-debug --target format
cmake --build --preset linux-native-debug --target format-check
```

Replace `linux-native-debug` with any configured build preset. If `clang-format` was installed after configuring CMake,
configure the preset again so CMake can add the formatting targets.

## Project layout

```text
cmake/      CMake helpers and cross-compilation toolchains
include/    Public, platform-independent headers
src/        Application and library implementation files
tests/      GoogleTest targets
```

## Run at startup

Copy `DoubleClickHotkey.exe` to `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup` to start it when signing in.

On Windows 11, hiding the console only works when the default terminal application is Windows Console Host. Change this
under `Settings > System > Advanced > Terminal`.

The utility is intended to be used with Logitech Onboard Memory Manager.
