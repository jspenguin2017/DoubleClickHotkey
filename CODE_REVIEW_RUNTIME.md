# Code Review: Runtime Behavior

## Reviewed scope and basis

- Scope: `src/application/`, `src/platform/windows/`, their public contracts under `include/double_click_hotkey/`, and
  `src/main.cpp`, with `README.md:3-33` used as the user-facing behavior contract.
- Revision: `46fa20a` (`main`), with a clean worktree confirmed before the review update began.
- Review method: revalidating the previous runtime findings by statically tracing launch parsing, service and
  control-client startup, single-instance ownership, command delivery, hook installation, F13 event classification and
  repeat handling, keyboard and mouse injection, error reporting, message multiplexing, and shutdown.
- API behavior was cross-checked against Microsoft's documentation for
  [`LowLevelKeyboardProc`](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc),
  [`SendInput`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput),
  [`ShowWindow`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow),
  [`ShowWindowAsync`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync), and
  [`HandlerRoutine`](https://learn.microsoft.com/en-us/windows/console/handlerroutine).

## Findings

### RUNTIME-4: Synchronous action and error reporting can make Windows silently remove the keyboard hook

- Severity: **Medium**
- Location: `src/platform/windows/keyboard_hook.cpp:60-86`, `src/application/application.cpp:110-125,133-148`,
  `src/platform/windows/windows_platform_binding.cpp:90-105,127-135`, `src/platform/windows/console.cpp:10-23`, and
  `src/platform/windows/mouse.cpp:9-32`
- Problem: The low-level hook invokes the portable handler synchronously. On the first F13 key-down, that handler calls
  `SendInput` to inject the double-click before returning. If injection fails, it also shows the console with
  `ShowWindow` and writes and flushes an error through `std::cout`, still before the hook returns. Microsoft requires a
  low-level hook to finish within `LowLevelHooksTimeout` (capped at one second on supported Windows versions); after a
  timeout, Windows can silently remove the hook. Window-visibility commands are now handled by the main event loop, so
  the normal show and hide paths no longer run in the hook; only double-click execution and its failure reporting remain
  synchronous there.
- Impact: If input injection or failure reporting stalls, the hook can disappear while the hidden service and its
  single-instance mutex remain alive. F13 then stops working with no hook-health error, while another normal launch only
  reports that the service is already running.
- Recommendation: Keep the hook callback limited to classifying and suppressing F13 transitions. Queue one action for
  the initial key-down and perform `SendInput` and any user-facing failure reporting from the event loop or a worker,
  while preserving the existing one-action-per-physical-press behavior.

## Unresolved questions

- How long do `SendInput` and the failure-reporting path take under load on the supported Windows 11 and Windows Console
  Host configuration? A Windows trace or fault-injection test is needed to quantify the remaining hook-timeout risk.
- Is interaction with elevated target windows intentionally unsupported? The application now detects an incomplete
  `SendInput` result and reports an error, but Windows does not identify UIPI as the cause and the README does not
  document the integrity-level limitation.

## Checks and areas not covered

- Reviewed launch dispatch and the portable controller, then traced every Windows platform-binding path against the
  README's F13, window-command, and single-instance contracts.
- `cmake --build --preset linux-native-debug` and `ctest --preset linux-native-debug --output-on-failure` passed all 22
  portable tests. They cover launch parsing and dispatch, service outcomes, window commands, repeat suppression, and
  injection-failure reporting through a fake binding, but not native injection or hook timing.
- The `linux-mingw-debug` and `linux-mingw-release` build presets completed successfully with MinGW-w64.
- Three previous findings no longer apply. F13 is consumed independently of modifier state, the controller suppresses
  repeats until key-up, and the mouse adapter verifies that all four `SendInput` events were inserted and propagates a
  failure for user-visible reporting. The former console-toggle portion of RUNTIME-4 also no longer applies because
  window visibility is controlled through commands handled outside the hook callback.
- No Windows desktop or input-injection harness is available in this Linux workspace, so the executable was not
  exercised against real keyboard, console-host, UIPI, hook-timeout, interprocess-command, or shutdown behavior.
- No application source, build configuration, or test file was modified as part of this review update.
