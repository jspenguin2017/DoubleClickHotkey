# Windows input review

## Scope and basis

- Repository-wide review, input segment: `src/platform/windows/input_threads.*`, `keyboard_hook.*`, `keyboard_sender.*`,
  `mouse.*`, `input_injector.*`, and `input_tag.hpp`, plus their controller and Windows binding call sites.
- Basis: clean starting worktree at commit `53a29b2de15af1c7ee7d81b84530588a066c7a54`, repository instructions, and the
  documented thread/input contract.
- Traced partial startup, worker readiness, queue creation, hook-thread confinement, request and diagnostic delivery,
  atomic failure publication, cancellation, thread joining, input tagging, and compensating releases.

## Findings

No verified findings in this segment. This does not establish that the code is defect-free.

## Unresolved questions

No additional implementation question was established by static review. Actual delivery and cancellation ordering under
native scheduling still require the Windows desktop checks below.

## Checks and limitations

- Compared callback ownership and work handoff against Microsoft's
  [LowLevelKeyboardProc contract](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc), and
  injection/error handling against the
  [SendInput contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).
- Checked startup key-state interpretation against
  [GetAsyncKeyState](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate).
- The existing, current native Linux test executable passed 55 portable tests. Those results do not exercise the Windows
  adapter.
- Interactive Windows 11 checks were unavailable on this Linux host: actual keyboard/mouse delivery, swapped buttons,
  cross-elevation input, UI suspension, secure-desktop transitions, partial injection failures, full queues, and
  shutdown during input remain unverified at runtime.
- No dependency installation, source modification, or review of individual test cases was performed.

## Review status

Complete.
