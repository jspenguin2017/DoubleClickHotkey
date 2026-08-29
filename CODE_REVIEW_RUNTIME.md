# Code Review: Runtime Behavior

## Reviewed scope and basis

- Scope: `DoubleClickHotkey.cpp`, with `README.md:3-11` used as the user-facing behavior contract.
- Revision: `42802b6` (`main`), with a clean worktree confirmed before the review began.
- Review method: static tracing of startup, hook installation, keyboard-event classification, mouse-event injection,
  message pumping, and shutdown.
- API behavior was cross-checked against Microsoft's documentation for
  [`LowLevelKeyboardProc`](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc),
  [`WM_KEYDOWN`](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keydown),
  [`SendInput`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput),
  [`ShowWindowAsync`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync), and
  [`HandlerRoutine`](https://learn.microsoft.com/en-us/windows/console/handlerroutine).

## Findings

### RUNTIME-1: Modifier changes can split an F8 press between the application and its target

- Severity: **Medium**
- Location: `DoubleClickHotkey.cpp:55-111`; behavior contract at `README.md:10-11`
- Problem: The hook independently recomputes whether to suppress each F8 key-down and key-up from the modifier state at
  that instant. For example, Ctrl+F8 key-down is forwarded because it matches neither feature, but if Ctrl is released
  before F8, the now-unmodified F8 key-up enters the double-click branch and is suppressed. The reverse sequence can
  deliver a lone key-up after the application suppressed the key-down.
- Impact: A foreground application can receive an unmatched F8 transition. Applications that maintain state from key
  messages can treat F8 as stuck down, miss its release, or process a release they never saw pressed. This also
  contradicts the README's promise that other applications do not receive F8 when a feature is activated.
- Recommendation: Decide the disposition of an F8 press on its first key-down and retain that state through all repeats
  and the matching key-up. Always forward or suppress the complete press as one unit, then reset the state on release.

### RUNTIME-2: Key auto-repeat invokes both hotkey actions repeatedly

- Severity: **Medium**
- Location: `DoubleClickHotkey.cpp:55-70`, `DoubleClickHotkey.cpp:90-104`
- Problem: Every `WM_KEYDOWN` or `WM_SYSKEYDOWN` for F8 performs an action. Windows emits multiple key-down messages
  before key-up when a key is held, so a held bare F8 generates repeated double-click sequences and a held
  Ctrl+Shift+Alt+F8 repeatedly flips console visibility.
- Impact: A slightly prolonged press can open or activate an item multiple times. The console toggle can flicker and
  finish in a state determined by the number of repeat messages rather than by one deliberate press.
- Recommendation: Track whether F8 is already down, run the action only on the up-to-down transition, consume repeat
  events according to the original press disposition, and clear the state on key-up. If repeat behavior is intentional,
  document it explicitly instead.

### RUNTIME-3: The synthesized double-click is neither validated nor submitted as one sequence

- Severity: **Medium**
- Location: `DoubleClickHotkey.cpp:90-106`
- Problem: The four mouse transitions are sent through four independent `SendInput` calls and every return value is
  ignored. `SendInput` reports how many events it inserted and is blocked by User Interface Privilege Isolation when the
  target has a higher integrity level. The API guarantees non-interleaving for events in one input array, but separate
  calls do not make the four-event click sequence one such batch. The hook suppresses F8 even when no mouse input was
  inserted.
- Impact: The primary workflow silently does nothing over an elevated target, or can produce a malformed click sequence
  if input interleaves or an individual call fails. The user receives no diagnostic and loses the original F8 event as
  well.
- Recommendation: Populate a four-element `INPUT` array, submit it in one `SendInput` call, and require a return value
  of four. Move failure reporting outside the hook callback, define what should happen when insertion is incomplete, and
  document the integrity-level limitation.

### RUNTIME-4: Synchronous window work can make Windows silently remove the keyboard hook

- Severity: **Medium**
- Location: `DoubleClickHotkey.cpp:33-41`, `DoubleClickHotkey.cpp:68-104`, `DoubleClickHotkey.cpp:135-161`
- Problem: The low-level hook callback directly calls `ShowWindow`/`HideConsole` and performs input injection before
  returning. Microsoft requires a low-level hook to finish within `LowLevelHooksTimeout` (capped at one second on
  current Windows); after a timeout, Windows can silently remove it. The console window is owned by another thread, and
  Microsoft specifically recommends `ShowWindowAsync` for changing a different thread's window state without waiting on
  it.
- Impact: If console-window processing stalls while the toggle is used, the hook can disappear while this hidden process
  and its single-instance mutex remain alive. F8 then stops working with no error, and launching another copy only
  reports that an instance is already running.
- Recommendation: Keep the hook limited to classifying and suppressing the key event. Queue the click or visibility
  action to other application code and return immediately; use an asynchronous cross-thread window-state operation for
  the console.

### RUNTIME-5: Control-handler cleanup races with main-thread hook ownership

- Severity: **Low**
- Location: `DoubleClickHotkey.cpp:7`, `DoubleClickHotkey.cpp:17-25`, `DoubleClickHotkey.cpp:43-46`,
  `DoubleClickHotkey.cpp:127-136`
- Problem: Windows runs a console control handler on a newly created thread. That thread reads, writes, and unhooks the
  global `KeyboardHookHandle` without synchronization, while the main thread can still be assigning it immediately after
  the handler is registered or can be entering another cleanup path. Concurrent access to this non-atomic object is a
  C++ data race, and `exit` begins process-wide C runtime teardown from the handler thread.
- Impact: A control signal during startup or simultaneous shutdown can cause undefined behavior, duplicate cleanup, or
  teardown while the main thread is still using process resources. The exposure window is small, which limits the
  severity, but the ownership contract is unsound.
- Recommendation: Give the main thread sole ownership of the hook and normal teardown. Have the control handler signal
  or post a quit request and return `TRUE`; synchronize any state that genuinely must be shared with the handler, with
  an OS-termination fallback for control events that cannot wait.

## Unresolved questions

- Does `GetKeyState` reliably expose all modifier transitions for the supported Windows versions and console hosts while
  the low-level hook's hidden thread is not the foreground input owner? Its documented state is queue-relative. A
  Windows trace should verify rapid modifier/F8 orderings; if results vary, maintain modifier state from hook events
  instead of querying it ad hoc.
- Is holding F8 intended to generate repeated double-clicks? The README describes one double-click per press, so this
  review treats auto-repeat as unintended.
- Is interaction with elevated target windows intentionally unsupported? Even if it is, the failure should be detected
  and the limitation documented.

## Checks and areas not covered

- Reviewed every production-code path and compared it with the README's hotkey contract.
- No tests or test infrastructure are tracked in the repository.
- No Windows C++ compiler (`cl`, `clang-cl`, MinGW), MSBuild installation, `cppcheck`, `clang-tidy`, Windows desktop, or
  input-injection harness is available in this Linux workspace, so the project could not be built, analyzed with those
  tools, or exercised against real keyboard, console-host, UIPI, or shutdown behavior.
- No dependencies were installed or changed, and no reviewed source or configuration file was modified.
