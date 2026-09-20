# Double Click Hotkey

Double Click Hotkey is a Windows 11 x64 utility that turns <kbd>F13</kbd> into a global double-click shortcut.

## Usage

Run `DoubleClickHotkey.exe` to start hidden in the notification area. Press <kbd>F13</kbd> to double-click. The app
consumes F13 events and allows one instance per interactive Windows session. It needs no separately installed runtime.

- Click the tray icon or choose **Show** to open the window. Launching the executable again also shows the running app.
- The read-only log keeps the newest 500 lines. Select text to copy it, or use <kbd>Ctrl</kbd>+<kbd>A</kbd> to select
  all.
- To configure F13 in another program, choose a delay in seconds, click **Send F13**, and focus that program before the
  countdown ends. The delay starts at five seconds on each launch.
- Closing the window hides it. Choose **Quit** from the tray menu to exit.

There are no command-line options. See [usage details](docs/usage.md) for countdowns, recovery, and elevation limits.

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
