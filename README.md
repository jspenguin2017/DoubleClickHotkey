# Double Click Hotkey

Double Click Hotkey is a small Windows 11 utility that turns <kbd>F13</kbd> into a global double-click shortcut.

## Behavior

Press <kbd>F13</kbd> to double-click. The application consumes F13 and performs the double-click regardless of whether
<kbd>Ctrl</kbd>, <kbd>Shift</kbd>, <kbd>Alt</kbd>, or either <kbd>Win</kbd> key is pressed.

The console window is hidden when the application starts normally. Use the command-line options below to show or hide
the running application's window; there is no keyboard shortcut for changing its visibility.

Only one hotkey-service instance can run at a time. The show and hide commands are short-lived control clients.

## Command line

The executable accepts one optional command-line argument:

```text
DoubleClickHotkey.exe --show
DoubleClickHotkey.exe --hide
DoubleClickHotkey.exe --send-f13
```

`--show` and `--hide` instruct the running instance to show or hide its console, then exit. They report an error when no
instance is running.

`--send-f13` requires that no instance is running. It waits five seconds and then sends one F13 press, making it
possible to select F13 as a hotkey in applications such as Logitech Onboard Memory Manager even though a typical
physical keyboard has no F13 key. The delay gives you time to focus the hotkey field in the target application.

Any invocation with a command-line argument keeps its own console visible. An invocation without arguments hides its
console immediately to avoid a startup flash.

## Platform support

The application supports only the latest generally available Windows 11 release. Older Windows releases are not
supported. It can be built with MinGW-w64 on Windows or cross-compiled from Linux, but the resulting executable is
intended to run only on Windows 11.

A native Linux build compiles the platform-independent application controller, then runs it through a fake platform
binding without creating the Windows application. This gives Linux development tools and CI a native target in addition
to the full MinGW-w64 cross-build.

The application depends on the abstract `PlatformBinding` interface rather than Win32 directly. The portable controller
dispatches launch commands, owns user-facing outcomes and timing, and applies hotkey and window-visibility policy. The
Windows binding only owns native service resources, event multiplexing, interprocess transport, and input/output calls.
CMake currently selects that binding, while the entry point and controller can be reused by future bindings.

## Requirements

All build environments need:

- CMake 3.28 or newer
- Ninja
- A compiler with C++17 support

Windows builds additionally need a 64-bit MinGW-w64 toolchain with `gcc` and `g++` available on `PATH`.

Linux development uses:

- `g++` for the native core and test build
- `x86_64-w64-mingw32-gcc`, `x86_64-w64-mingw32-g++`, and `x86_64-w64-mingw32-windres` for the Windows cross-build

On Debian or Ubuntu, the C++ build tools can be installed with:

```sh
sudo apt install clang-format cmake g++ mingw-w64 ninja-build
```

GoogleTest does not need a separate installation. Debug configurations download the pinned GoogleTest dependency the
first time CMake configures the project, so that first configuration needs network access.

Document formatting needs Node.js 20 or newer and npm. Node.js 22.12 or newer is recommended for development.
`clang-format` is only required for the C++ formatting targets and is not required to compile the project.

## `npm` shortcuts

After installing the locked npm dependencies with `npm install`, the following commands select the appropriate CMake
presets for the host platform:

```sh
npm run build
npm run test
npm run format
npm run format:check
```

`npm run build` creates both Debug and Release Windows application builds. On Linux it uses the MinGW-w64 cross-build
presets; on Windows it uses the native MinGW-w64 presets. `npm run test` builds and runs the portable native tests on
Linux or the Windows tests on Windows. The formatting commands run both Prettier and the corresponding CMake
`clang-format` target.

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

Tests use GoogleTest 1.18.0, pinned to a specific upstream revision and archive checksum. Test sources are grouped by
domain under `tests/` and link against the platform-independent `DoubleClickHotkey::core` CMake target.

Portable application behavior belongs in `double_click_hotkey_core`. Native APIs belong in a platform adapter such as
`src/platform/windows/`, which implements `PlatformBinding` and supplies the platform-binding factory. The contract is
at the service level: an adapter owns its native event loop and resource lifetimes, while reporting semantic events and
operation outcomes to the controller. To support another operating system, add its adapter and select its sources and
native libraries in the platform section of `CMakeLists.txt`; the application controller and `src/main.cpp` do not need
platform-specific changes.

## Formatting

Install the locked document formatter once:

```sh
npm install
```

Format supported documents, configuration files, and C++ sources with:

```sh
npm run format
npm run format:check
```

The npm commands configure the platform's development preset before invoking CMake. To run the C++ formatting targets
directly after configuring a preset, use:

```sh
cmake --build --preset linux-native-debug --target format
cmake --build --preset linux-native-debug --target format-check
```

Replace `linux-native-debug` with any configured build preset.

## Project layout

```text
cmake/                  CMake helpers and cross-compilation toolchains
include/                Public, platform-independent contracts
src/application/        Application orchestration
src/platform/windows/   Win32 platform binding and native service adapters
src/main.cpp             Platform-neutral composition root
scripts/                 Cross-platform npm command helpers
tests/                   Domain-grouped GoogleTest coverage
```

## Run at startup

Copy `DoubleClickHotkey.exe` to `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup` to start it when signing in.

On Windows 11, hiding the console only works when the default terminal application is Windows Console Host. Change this
under `Settings > System > Advanced > Terminal`.

The utility is intended to be used with Logitech Onboard Memory Manager.
