# Windows input and instance activation code review

## Scope and basis

- Repository-wide review segment: `src/platform/windows/keyboard_hook.*`,
  `keyboard_sender.*`, `input_injector.*`, `input_tag.hpp`, `mouse.*`, `clock.*`,
  `single_instance.*`, and `named_object_security.hpp`.
- Also traced their integration in `windows_platform_binding.cpp`, `main_window.cpp`,
  the portable controller, and `docs/usage.md`.
- Basis: commit `c502829e0f39009868ec79818186779d42e6bb6b`; initially clean worktree.
- Reviewed input consumption and repeat suppression, tagged F13 passthrough, balanced input
  sequences and partial-send cleanup, error reporting, elapsed-time semantics, instance
  readiness, named-object permissions, duplicate activation, and resource lifetime.

## Findings

No verified findings in this segment. This does not establish that the code is defect-free.

## Unresolved questions

- Duplicate Show requests are serviced by the outer wait loop at
  `src/platform/windows/windows_platform_binding.cpp:35-40`. A request arriving while the
  thread is inside a native modal menu or move/resize loop can remain pending until that loop
  exits. Confirm whether this delay is acceptable; this review did not reproduce an ordinary
  launch workflow that leaves the application inaccessible.
- The hook shares the UI thread. There is no demonstrated slow production path in this review,
  but Windows can silently remove a low-level hook when its dispatch exceeds the timeout.
  Runtime validation under a stalled Explorer or heavy UI activity remains useful.
  [Microsoft's low-level keyboard hook contract](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc).

## Checks and limitations

- Traced both input sequences and their compensating release paths; confirmed that generated
  F13 down/up events carry the same process-local tag checked by the hook.
- Checked the interaction between hook-side press tracking, queued native messages, and the
  controller's press tracking; native callbacks perform no allocating application work.
- Inspected mutex/event creation rights, session-local names, two-second readiness retry,
  Show-only signaling, and shutdown destruction order. The documented exclusions for native
  process interference and the tag's non-authentication role were respected.
- Verified existing Release build freshness with a Ninja dry run. Binary inspection is
  recorded in the build/tooling report.
- No Windows desktop or Wine is available. Actual input delivery, partial SendInput behavior,
  swapped-button operation, cross-elevation activation, secure-desktop transitions, and
  suspend/resume timing were not exercised.
- Third-party implementation code and individual portable test cases were not reviewed.
