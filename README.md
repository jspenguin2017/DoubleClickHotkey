# Double Click Hotkey

Double Click Hotkey is a small Windows 11 utility that turns <kbd>F13</kbd> into a global double-click shortcut.

## Usage

Run `DoubleClickHotkey.exe` and press <kbd>F13</kbd> to double-click. The application consumes F13 events.

The service allows one instance per interactive Windows session, so users in different sessions can each run their own
instance. It hides its console at startup unless `--start-shown` is used. The other commands are one-shot operations and
keep their own console visible:

| Command                               | Effect                                                                                        |
| ------------------------------------- | --------------------------------------------------------------------------------------------- |
| `DoubleClickHotkey.exe --start-shown` | Starts the service with its console shown instead of hidden.                                  |
| `DoubleClickHotkey.exe --show`        | Shows the running service's console, or reports that no ready instance is available.          |
| `DoubleClickHotkey.exe --hide`        | Hides the running service's console, or reports that no ready instance is available.          |
| `DoubleClickHotkey.exe --send-f13`    | With no service running, waits five seconds and sends one F13 press for hotkey configuration. |

Double-click injection errors are logged without revealing a hidden console; use `--show` to inspect them. A `--show` or
`--hide` command issued while the service is still initializing may fail; use `--start-shown` when the console must be
visible from the beginning.

Showing and hiding the console are intentionally not treated as sensitive operations. Within the same interactive
session, a non-elevated launch may show or hide a service that was started elevated. This permission does not grant the
caller the ability to inject input through the elevated service.

[Windows restricts synthesized input](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput)
to applications at an equal or lower integrity level. Double Click Hotkey therefore cannot double-click or send F13 to
an elevated application unless it is running at the same or a higher integrity level. Windows does not identify when
this restriction caused an injection failure, so a failure without an error code reports an integrity-level mismatch as
a possible cause rather than a certainty.

## Security model

Double Click Hotkey is designed to resist interference from an ordinary webpage running in a secure, uncompromised
browser; browser extensions are not part of the threat model. Denial of service by a native process in the same
interactive session is outside the threat model. The cross-process command channel accepts only `--show` and `--hide`,
which are not treated as sensitive operations; the channel cannot be used to request F13 or mouse input injection.

## Platform support

The executable only supports Windows 11. It can be built with MinGW-w64 on Windows or cross-compiled from Linux. Linux
also has a native target for testing the platform-independent controller.

## Requirements

All workflows require:

- CMake 3.28 or newer
- Ninja
- A compiler with C++17 support

The npm workflows additionally require Node.js 24.19.0 or newer and npm. Windows builds need a 64-bit MinGW-w64
toolchain; on Linux, the default cross-compilation tools are `x86_64-w64-mingw32-gcc`, `x86_64-w64-mingw32-g++`, and
`x86_64-w64-mingw32-windres`.

On Ubuntu 24, the C++ build tools can be installed with:

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
