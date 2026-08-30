# Code Review: Windows Platform

## Reviewed scope and basis

- Scope: `src/platform/windows/` and the public platform contracts it implements in
  `include/double_click_hotkey/platform_binding.hpp` and `include/double_click_hotkey/platform_factory.hpp`.
- Review basis: repository state at commit `9e03888e53b069b9f0dd2ffdeef8ba97c9774daf` on 2026-08-30.
- Review method: traced native resource ownership, startup/shutdown, console-control delivery, single-instance state,
  cross-process show/hide commands, low-level keyboard-hook dispatch, message/event multiplexing, and synthesized
  keyboard and mouse input. Win32 semantics were checked against the linked Microsoft API documentation.

## Findings

### W-1: A control client can be mistaken for the running service

- Severity: **Medium**
- References: `src/platform/windows/single_instance.cpp:5-17`,
  `src/platform/windows/windows_platform_binding.cpp:31-40`, and
  `src/platform/windows/windows_platform_binding.cpp:67-73`.
- Problem: `SendWindowCommand` probes for the service by constructing `SingleInstance`. That constructor calls
  `CreateMutexW(..., TRUE, name)`, so when no mutex exists, a `--show` or `--hide` client creates and owns the same mutex
  used to reserve the service. A service starting during the control client's short lifetime receives
  `ERROR_ALREADY_EXISTS` and exits as though another service were running. The client simultaneously returns
  `not_running`; once it exits, neither process owns the service reservation. This follows directly from
  [the documented create-or-open and initial-ownership behavior of `CreateMutexW`](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createmutexw).
- Impact: A concurrent visibility command can prevent a legitimate startup, leaving no hotkey service running until the
  user starts it again. The race is especially plausible around login/startup automation and scripted control commands.
- Recommendation: Separate reservation from observation. Have control clients use `OpenMutexW` (or open the receiver's
  IPC endpoint directly) so they never create or own the service mutex. Treat a missing object as `not_running`, and map
  shutdown races consistently to the same semantic outcome.

### W-2: Show/hide IPC does not preserve command order

- Severity: **Medium**
- References: `src/platform/windows/instance_command.cpp:35-42`,
  `src/platform/windows/instance_command.cpp:60-63`, `src/platform/windows/instance_command.cpp:70-82`, and
  `src/platform/windows/windows_platform_binding.cpp:167-185`.
- Problem: Show and hide are represented by separate auto-reset events in a fixed array (`show` at index 0 and `hide` at
  index 1). If a hide command is followed by a show command before the service consumes either event, both handles are
  signaled. The wait checks handles in array order, so the service consumes show first and hide second, ending hidden
  even though show was the latest command. Auto-reset events also retain only signaled/nonsignaled state rather than a
  command sequence. Microsoft documents both [auto-reset event behavior](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventw)
  and [the first-signaled-handle selection rule for message waits](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjects).
- Impact: Closely spaced opposing control commands can produce a final console state that contradicts invocation order,
  which makes scripting and rapid user commands nondeterministic.
- Recommendation: Use an IPC mechanism that carries an ordered command stream, such as a named pipe, or a single wake
  event paired with shared state containing a monotonically increasing sequence and the latest requested visibility.
  Ensure the receiver applies commands in send order or explicitly implements last-write-wins semantics.

### W-3: Partial input injection can leave a key or mouse button pressed

- Severity: **Medium**
- References: `src/platform/windows/keyboard_sender.cpp:17-23` and `src/platform/windows/mouse.cpp:22-28`.
- Problem: Both senders correctly detect when `SendInput` inserts fewer records than requested, but they return
  immediately without compensating for the successfully inserted prefix. `SendInput` returns the number of records it
  inserted, not an all-or-nothing result. If only the F13-down record or an odd-numbered prefix of the mouse sequence is
  inserted, the matching key-up or left-button-up record is absent. See Microsoft's
  [`SendInput` return-value contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).
- Impact: A partial failure can leave F13 or the primary mouse button logically held, causing unintended shortcuts,
  selection, or dragging until another release transition happens to repair the state.
- Recommendation: Use the returned count to determine whether the inserted prefix ends in a down transition. If it does,
  immediately attempt a minimal compensating key-up or button-up injection and report both the original short write and
  any recovery failure. Structure each operation so release cleanup is attempted on every failure path.

### W-4: UIPI-blocked input can be reported as error code zero

- Severity: **Low**
- References: `src/platform/windows/keyboard_sender.cpp:17-23`, `src/platform/windows/mouse.cpp:22-28`, and
  `src/platform/windows/windows_platform_binding.cpp:117-141`.
- Problem: The senders clear the thread's last-error value before calling `SendInput`, then propagate `GetLastError()`
  whenever the inserted count is short. Windows explicitly states that neither the return value nor `GetLastError`
  identifies a failure caused by User Interface Privilege Isolation (UIPI). Consequently, an attempt to target an
  elevated application can leave the cleared value unchanged and be logged only as `error code: 0`, which reads like
  success rather than a useful failure.
  See the [Microsoft `SendInput` UIPI documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).
- Impact: Users cannot diagnose why the utility fails against a higher-integrity target and receive no actionable hint
  about the application's integrity-level limitation.
- Recommendation: When injection is short and the captured error remains `ERROR_SUCCESS`, emit an explicit fallback
  diagnostic explaining that Windows blocked or otherwise rejected the input and that an integrity-level mismatch is a
  possible cause. Document the elevated-target limitation without claiming UIPI can be detected conclusively.

## Unresolved questions

- `KeyboardHook::Install` passes `nullptr` for `hMod` and zero for `dwThreadId` at
  `src/platform/windows/keyboard_hook.cpp:36`. `WH_KEYBOARD_LL` is global-only, and Microsoft says
  [`SetWindowsHookExW` may fail when this parameter combination is used](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowshookexw).
  The hook could not be exercised on the Linux review host. Confirm this exact call on every supported Windows 11 build;
  if the null module handle is not deliberately validated, use the executable module handle from
  `GetModuleHandleW(nullptr)`.

## Checks and areas not covered

- `npm run build` configured and built both `linux-mingw-debug` and `linux-mingw-release` successfully with MinGW-w64
  GCC 13.
- The generated Windows executables were not run because no Windows or compatible runtime was available on the review
  host. Hook installation, named-object ACL behavior, console-host visibility, actual input injection, and shutdown
  delivery therefore remain runtime-unverified.
- The cross-compiled GoogleTest executable was built but not run, consistent with the repository's Linux preset model.
- Individual test cases, fixtures, assertions, and coverage adequacy were not reviewed.
