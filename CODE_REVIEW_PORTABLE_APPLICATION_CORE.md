# Code Review: Portable Application Core

## Reviewed scope and basis

- Scope: `include/double_click_hotkey/`, `src/application/`, and `src/main.cpp`.
- Review basis: repository state at commit `9e03888e53b069b9f0dd2ffdeef8ba97c9774daf` on 2026-08-30.
- Review method: traced launch parsing, command dispatch, service callbacks, window-visibility policy, single-instance
  outcomes, delayed F13 injection, hotkey state transitions, error reporting, platform-factory ownership, and the
  contracts between `Application` and `PlatformBinding`.

## Findings

No verified findings were identified in this segment. This means the review did not find an actionable defect in the
reviewed portable code; it does not establish that the segment is defect-free.

## Unresolved questions

None.

## Checks and areas not covered

- `cmake --build --preset linux-native-debug` completed successfully (`ninja: no work to do`).
- `ctest --preset linux-native-debug` passed all 27 discovered tests.
- Individual test cases, fixtures, assertions, and coverage adequacy were not reviewed, as required by the review scope's
  test guardrails.
- Native Win32 behavior and its concrete `PlatformBinding` implementation are covered separately in
  `CODE_REVIEW_WINDOWS_PLATFORM.md`.
- No runtime Windows exercise was performed from the Linux review host.
