# Windows input and instance activation code review

## Scope and basis

- Repository-wide review segment: `src/platform/windows/keyboard_hook.*`, `keyboard_sender.*`, `input_injector.*`,
  `input_tag.hpp`, `mouse.*`, `clock.*`, `single_instance.*`, and `named_object_security.hpp`.
- Also traced their integration in `windows_platform_binding.cpp`, `main_window.cpp`, the portable controller, and
  `docs/usage.md`.
- Basis: commit `c502829e0f39009868ec79818186779d42e6bb6b`; initially clean worktree.
- Findings and unresolved questions rechecked against commit `7c275c98bff93fad771e53db6b8f88d9d6b374a4`.
- Reviewed input consumption and repeat suppression, tagged F13 passthrough, balanced input sequences and partial-send
  cleanup, error reporting, elapsed-time semantics, instance readiness, named-object permissions, duplicate activation,
  and resource lifetime.

## Findings

No verified findings in this segment. This does not establish that the code is defect-free.

## Unresolved questions

- Duplicate Show requests are serviced by the outer wait loop at
  `src/platform/windows/windows_platform_binding.cpp:37-44`. A request arriving while the thread is inside a native
  modal menu or move/resize loop can remain pending until that loop exits. Confirm whether this delay is acceptable;
  this review did not reproduce an ordinary launch workflow that leaves the application inaccessible.

## Original review checks and limitations

- Traced both input sequences and their compensating release paths; confirmed that generated F13 down/up events carry
  the same process-local tag checked by the hook.
- Inspected mutex/event creation rights, session-local names, two-second readiness retry, Show-only signaling, and
  shutdown destruction order. The documented exclusions for native process interference and the tag's non-authentication
  role were respected.
- No Windows desktop or Wine is available. Actual input delivery, partial SendInput behavior, swapped-button operation,
  cross-elevation activation, secure-desktop transitions, and suspend/resume timing were not exercised.
- Third-party implementation code and individual portable test cases were not reviewed.
