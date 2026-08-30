# Double Click Hotkey

Double Click Hotkey is a small Windows 11 utility that turns <kbd>F13</kbd> into a global double-click shortcut.

## Usage

Run `DoubleClickHotkey.exe` and press <kbd>F13</kbd> to double-click. The application consumes F13 even when
<kbd>Ctrl</kbd>, <kbd>Shift</kbd>, <kbd>Alt</kbd>, or either <kbd>Win</kbd> key is held.

The service allows only one instance and hides its console at startup. These one-shot commands keep their own console
visible:

| Command                            | Effect                                                                                        |
| ---------------------------------- | --------------------------------------------------------------------------------------------- |
| `DoubleClickHotkey.exe --show`     | Shows the running service's console, or reports that no instance is running.                  |
| `DoubleClickHotkey.exe --hide`     | Hides the running service's console, or reports that no instance is running.                  |
| `DoubleClickHotkey.exe --send-f13` | With no service running, waits five seconds and sends one F13 press for hotkey configuration. |

Injection errors are logged without revealing a hidden console; use `--show` to inspect them. There is no hotkey for
changing console visibility.

## Platform support

The executable supports only the latest generally available Windows 11 release. It can be built with MinGW-w64 on
Windows or cross-compiled from Linux. Linux also has a native target for testing the platform-independent controller.

## Requirements

All workflows require:

- CMake 3.28 or newer
- Ninja
- A compiler with C++17 support

The npm workflows additionally require Node.js 24.19.0 or newer and npm. Windows builds need a 64-bit MinGW-w64
toolchain; on Linux, the default cross-compilation tools are `x86_64-w64-mingw32-gcc`, `x86_64-w64-mingw32-g++`, and
`x86_64-w64-mingw32-windres`.

On Debian or Ubuntu, the C++ build tools can be installed with:

```sh
sudo apt install clang-format cmake g++ mingw-w64 ninja-build
```

`clang-format` is optional unless formatting C++ sources. Debug presets download a pinned GoogleTest dependency during
their first configuration, which requires network access.

## Build, test, and format

The npm shortcuts select the appropriate CMake presets for Linux or Windows:

```sh
npm install
npm run build
npm run test
npm run format
npm run format:check
```

`npm run build` creates both Debug and Release Windows application builds. On Linux it uses the MinGW-w64 cross-build
presets; on Windows it uses native MinGW-w64 presets. `npm run test` builds and runs the host's test preset. The format
commands run Prettier and the CMake `clang-format` target.

For direct CMake use, configure and build one of these presets:

| Host    | Preset                  | Purpose                             |
| ------- | ----------------------- | ----------------------------------- |
| Linux   | `linux-native-debug`    | Build and run portable tests        |
| Linux   | `linux-mingw-debug`     | Cross-build the Windows debug app   |
| Linux   | `linux-mingw-release`   | Cross-build the Windows release app |
| Windows | `windows-mingw-debug`   | Build and run Windows tests         |
| Windows | `windows-mingw-release` | Build the Windows release app       |

```sh
cmake --preset linux-native-debug
cmake --build --preset linux-native-debug
ctest --preset linux-native-debug
```

Substitute the desired preset. Only `linux-native-debug` and `windows-mingw-debug` have CTest presets; Linux cannot run
the cross-compiled Windows tests directly. Build output is under `build/<preset>`, and release executables are
statically linked at `build/<release-preset>/DoubleClickHotkey.exe`.

The Linux cross-toolchain defaults to `x86_64-w64-mingw32`; override `MINGW_W64_TRIPLE` during initial configuration if
your toolchain uses another prefix. C++ formatting targets can also be invoked directly after configuration:

```sh
cmake --build --preset linux-native-debug --target format
cmake --build --preset linux-native-debug --target format-check
```

## Run at startup

Copy `DoubleClickHotkey.exe` to `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup` to start it when signing in.

On Windows 11, hiding the console only works when the default terminal application is Windows Console Host. Change this
under `Settings > System > Advanced > Terminal`.
