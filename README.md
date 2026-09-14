# Double Click Hotkey

Double Click Hotkey is a Windows 11 x64 utility that turns <kbd>F13</kbd> into a global double-click shortcut.

## Usage

Run `DoubleClickHotkey.exe` and press <kbd>F13</kbd> to double-click. The app consumes F13 events, hides its console,
and allows one instance per interactive Windows session. It needs no separately installed runtime.

Launch with one of these options when needed:

| Option          | Effect                                                                    |
| --------------- | ------------------------------------------------------------------------- |
| `--start-shown` | Start the service with its console visible.                               |
| `--show`        | Show the running service's console and logged errors.                     |
| `--hide`        | Hide the running service's console.                                       |
| `--send-f13`    | With no service running, wait five seconds and send F13 for hotkey setup. |

See [usage details](docs/usage.md) for console setup, command failures, elevation limits, and the security model.

## Run at startup

Copy `DoubleClickHotkey.exe` to `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup` to start it when signing in.

## Build, test, and format

Install the [prerequisites](docs/development.md#prerequisites), then run from the repository root:

```sh
npm install
npm run build
npm run test
npm run format
npm run format:check
```

`build` creates Debug and Release Windows executables at `build/<preset>/DoubleClickHotkey.exe`, cross-compiling on
Linux. `test` runs portable tests on the current host. Formatting uses Prettier and clang-format.

See [development](docs/development.md) for direct CMake commands and the repository layout,
[release size and validation](docs/release.md) for binary checks, and [AGENTS.md](AGENTS.md) for contribution
constraints.
