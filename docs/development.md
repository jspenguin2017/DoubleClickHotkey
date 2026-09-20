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
  `include/double_click_hotkey/`; portable log retention, countdown state, hotkey policy, and orchestration live in
  `src/application/`.
- `src/platform/windows/` implements the native window, tray, DPI handling, Show-only instance activation, keyboard
  hooks and sending, mouse input, and the monotonic clock.
- `tests/application/` tests the core through the shared `fake_platform_binding.hpp`, without real sleeps or desktop
  interaction. `tests/main.test.cpp` supplies a fake platform factory; `tests/CMakeLists.txt` builds the entrypoint
  under a test-only function name.
- `CMakeLists.txt` defines targets, GoogleTest, warnings, and size flags; `cmake/` holds formatting and toolchain setup.
  `package.json`, `scripts/cmake.mjs`, and `.prettier*` configure npm workflows and documentation formatting.

## Thread ownership

The Windows adapter owns three application threads at normal priority:

| Thread        | Owns                                                                   | Handoff                                                                   |
| ------------- | ---------------------------------------------------------------------- | ------------------------------------------------------------------------- |
| Main/UI       | Application controller, window, tray, countdown, logs, keyboard sender | Receives double-click diagnostics and worker failures                     |
| Keyboard hook | Hook installation, message pump, portable `HotkeyPolicy`, unhooking    | Posts one double-click request per F13 press directly to the input thread |
| Input         | Mouse sender and compensating mouse-button releases                    | Posts numeric error data to the UI window; successful clicks are silent   |

`InputThreads` uses `_beginthreadex` and owned Windows events/handles, retaining compatibility with the MinGW win32
thread model. It creates the input message queue before signaling readiness, then starts and installs the hook on its
own thread. Instance readiness is published after both workers and the initial UI presentation are ready.

The hook callback never allocates, formats text, waits for either thread, sends input, or invokes an application
callback. It hands work off with `PostThreadMessageW`, following
[Microsoft's low-level hook guidance](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc). Only
the hook thread touches its key state. Windows message queues provide FIFO delivery; a failed post is a fatal service
error, not a retry inside the callback. Worker failures publish an atomic error and signal a separate event in the main
wait loop, with an additional window message for native modal loops. Cross-thread messages contain no owning pointers.
Input failure text is formatted and logged on the UI thread.

The UI still sends F13 synchronously. Both generated transitions, including any compensating release after a partial
send, carry the same immutable process-local tag. `HotkeyPolicy` passes matching injected input through before changing
any physical-key state. No shared suppression flag or temporary unhooking is used, so physical holds and startup
passthrough survive interleaved setup keystrokes. Keyboard and mouse injection have separate error state.

Quit immediately cancels the countdown and signals input cancellation. Queued clicks are discarded; a sequence already
in progress may finish its compensating release. After UI callbacks unwind, cleanup detaches callbacks, joins the input
thread while the hook still pumps, then stops and joins the hook thread before destroying the window. The instance lock
is retained until cleanup finishes. Partial startup failures use the same teardown order.

### Windows thread validation

Portable tests exercise normalized hotkey decisions and controller behavior without desktop APIs or real sleeps. They do
not validate native scheduling or actual input delivery. On a Windows 11 desktop, also check:

1. Confirm the main, hook, and input thread entrypoints run on distinct threads. Suspend only the UI thread for several
   seconds while testing F13 against another responsive application. Double-clicks must continue during the stall and
   after the UI resumes. Repeat with tray menus, resizing, and Explorer restart.
2. Capture **Send F13** in another application: both transitions must arrive without a double-click. Repeat while
   physical F13 is held and when F13 was held at startup; physical auto-repeat must not produce extra clicks. Check
   ordinary untagged injected F13 and swapped mouse buttons too.
3. Exercise thread-creation/hook-installation failure and failed request/diagnostic posting using a debugger. The
   service must exit with a diagnostic, unwind partial startup, and release the instance lock. A full UI queue must not
   prevent the failure event from being observed once the UI resumes.
4. Quit with clicks queued and during countdowns, including from a modal menu and during session shutdown. Queued clicks
   must not be replayed during teardown, input releases must finish, and worker handles must be joined before the HWND
   is destroyed. Repeated launch/quit cycles must not leave hooks or worker threads behind.

## Icon and native resources

`assets/icon.svg` is the original editable artwork. After editing it, run `npm run icon:convert` to regenerate the
checked-in `assets/icon.ico` with a single PNG-compressed 256×256 image to reduce resource size. Windows scales it for
smaller tray and window icons. The converter is a development dependency; application builds use the checked-in ICO and
require no Node.js runtime or image conversion.

The Windows resource embeds the icon, version information, and an as-invoker Per-Monitor V2 manifest. CMake explicitly
tracks these inputs so resource edits trigger incremental rebuilds. The executable uses the Windows GUI subsystem, while
`src/main.cpp` stays platform neutral and takes no arguments.
