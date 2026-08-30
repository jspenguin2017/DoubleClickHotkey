# Code Review: Runtime Behavior

## Reviewed scope and basis

- Scope: `src/application/application.cpp`, `src/hotkey/hotkey_action.cpp`, `src/platform/windows/`, their public
  contracts under `include/double_click_hotkey/`, and `src/main.cpp`, with `README.md:3-13` used as the user-facing
  behavior contract.
- Revision: `985365d` (`main`), with a clean worktree confirmed before the review update began.
- Review method: revalidating the previous runtime findings by statically tracing startup, hook installation,
  keyboard-event classification, portable application dispatch, mouse-event injection, message pumping, and shutdown
  across the restructured components.
- API behavior was cross-checked against Microsoft's documentation for
  [`LowLevelKeyboardProc`](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc),
  [`WM_KEYDOWN`](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-keydown),
  [`SendInput`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput),
  [`ShowWindowAsync`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindowasync), and
  [`HandlerRoutine`](https://learn.microsoft.com/en-us/windows/console/handlerroutine).

## Findings

### RUNTIME-1: Modifier changes can split an F8 press between the application and its target

- Severity: **Medium**
- Location: `src/platform/windows/keyboard_hook.cpp:60-99`, `src/application/application.cpp:16-41`; behavior contract
  at `README.md:7-11`
- Problem: The Windows hook captures the current modifier state independently for every F8 transition, and the
  application recomputes the action for each event without retaining a per-press disposition. For example, Ctrl+F8
  key-down is forwarded because it matches neither shortcut, but releasing Ctrl before F8 makes the unmodified F8 key-up
  enter the double-click branch and be suppressed. The reverse sequence can deliver a lone key-up after the application
  suppressed the key-down.
- Impact: A foreground application can receive an unmatched F8 transition. Applications that maintain state from key
  messages can treat F8 as stuck down, miss its release, or process a release they never saw pressed. This also
  contradicts the README's promise that other applications do not receive F8 when a shortcut is activated.
- Recommendation: Decide the disposition of an F8 press on its first key-down and retain that state through all repeats
  and the matching key-up. Always forward or suppress the complete press as one unit, then reset the state on release.

### RUNTIME-2: Key auto-repeat invokes both hotkey actions repeatedly

- Severity: **Medium**
- Location: `src/platform/windows/keyboard_hook.cpp:70-99`, `src/application/application.cpp:16-39`
- Problem: The hook maps every `WM_KEYDOWN` or `WM_SYSKEYDOWN` for F8 to `KeyTransition::pressed`, and the application
  performs the selected action for every such event. Windows emits repeated key-down messages before key-up while a key
  is held, so a held bare F8 generates repeated double-click sequences and a held Ctrl+Shift+Alt+F8 repeatedly flips
  console visibility.
- Impact: A slightly prolonged press can open or activate an item multiple times. The console toggle can flicker and
  finish in a state determined by the number of repeat messages rather than by one deliberate press.
- Recommendation: Track whether F8 is already down, run the action only on the up-to-down transition, consume repeat
  events according to the original press disposition, and clear the state on key-up. If repeat behavior is intentional,
  document it explicitly instead.

### RUNTIME-3: Mouse-injection failures are silently ignored

- Severity: **Medium**
- Location: `src/platform/windows/mouse.cpp:9-23`, `src/platform/windows/windows_platform_binding.cpp:46-49`,
  `src/application/application.cpp:24-41`
- Problem: The four mouse transitions are now submitted together in one `SendInput` call, but its return value is
  discarded and the platform interface cannot report failure. `SendInput` reports how many events it inserted and is
  blocked by User Interface Privilege Isolation when the target has a higher integrity level. The application suppresses
  F8 even if the complete mouse sequence was not inserted.
- Impact: The primary workflow can silently do nothing over an elevated target or produce an incomplete click sequence.
  The user receives no diagnostic and loses the original F8 event as well.
- Recommendation: Make mouse injection report whether all four events were inserted, propagate or asynchronously report
  failures outside the hook callback, define what should happen after incomplete insertion, and document the
  integrity-level limitation.

### RUNTIME-4: Synchronous window work can make Windows silently remove the keyboard hook

- Severity: **Medium**
- Location: `src/platform/windows/keyboard_hook.cpp:90-96`, `src/application/application.cpp:16-39`,
  `src/platform/windows/windows_platform_binding.cpp:46-54`, `src/platform/windows/console.cpp:10-35`, and
  `src/platform/windows/mouse.cpp:9-23`
- Problem: The low-level hook invokes the portable handler synchronously, and that handler directly performs input
  injection or calls `ShowWindow` before returning. Microsoft requires a low-level hook to finish within
  `LowLevelHooksTimeout` (capped at one second on current Windows); after a timeout, Windows can silently remove it. The
  console window is owned by another thread, and Microsoft specifically recommends `ShowWindowAsync` for changing a
  different thread's window state without waiting on it.
- Impact: If console-window processing stalls while the toggle is used, the hook can disappear while this hidden process
  and its single-instance mutex remain alive. F8 then stops working with no error, and launching another copy only
  reports that an instance is already running.
- Recommendation: Keep the hook limited to classifying and suppressing the key event. Queue the click or visibility
  action to work performed outside the callback, and use an asynchronous cross-thread window-state operation for the
  console.

## Unresolved questions

- Does `GetKeyState` reliably expose all modifier transitions for the supported Windows version and console host while
  the hook thread is not the foreground input owner? Its documented state is queue-relative. A Windows trace should
  verify rapid modifier/F8 orderings; if results vary, maintain modifier state from hook events instead of querying it
  ad hoc.
- Is holding F8 intended to generate repeated double-clicks? The README describes one double-click per press, so this
  review treats auto-repeat as unintended.
- Is interaction with elevated target windows intentionally unsupported? Even if it is, the failure should be detected
  and the limitation documented.

## Checks and areas not covered

- Reviewed the portable controller and every Windows platform-binding path, then compared them with the README's hotkey
  contract.
- `npm run test` passed all 10 portable application and hotkey-policy tests. Those tests cover action selection and
  basic dispatch but not native key repeat, modifier changes within one press, input injection, hook timing, or
  shutdown.
- The `linux-mingw-debug` and `linux-mingw-release` configure/build presets passed with MinGW-w64.
- The previous four-call input sequence is now one `SendInput` batch, eliminating that portion of the old injection
  finding. The previous control-handler ownership race no longer applies: the handler atomically reads the main-thread
  ID and posts `WM_QUIT`, while main-thread RAII objects own normal teardown.
- No Windows desktop or input-injection harness is available in this Linux workspace, so the executable was not
  exercised against real keyboard, console-host, UIPI, hook-timeout, or shutdown behavior.
- No application source, build configuration, or test file was modified as part of this review update.
