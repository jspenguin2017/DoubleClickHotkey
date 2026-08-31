# Code Review: Application Core

## Reviewed Scope and Basis

Reviewed the portable application/core segment:

- `include/double_click_hotkey/application.hpp`
- `include/double_click_hotkey/launch_command.hpp`
- `include/double_click_hotkey/platform_binding.hpp`
- `include/double_click_hotkey/platform_factory.hpp`
- `src/application/application.cpp`
- `src/application/launch_command.cpp`
- `src/main.cpp`

The review basis is the current clean worktree at the start of the review, the contracts and data flows within the files
above, and directly related CMake, Windows adapter, test-infrastructure, and README material inspected to validate those
contracts. Third-party, generated, and vendored code are excluded.

## Review Progress

- Launch-command parsing: reviewed; no findings identified.
- Application orchestration and portable platform contract: reviewed; no findings identified.
- Composition-root integration: reviewed; no findings identified.

## Findings

No findings were identified in the reviewed segment. This does not imply that the code is defect-free.

## Unresolved Questions

None at this milestone.

## Checks and Areas Not Covered

- `cmake --build --preset linux-native-debug --target double_click_hotkey_tests` completed successfully
  (`ninja: no work to do`), confirming the configured native test target is current.
- `ctest --preset linux-native-debug --output-on-failure` passed all 30 discovered tests.
- `cmake --build --preset linux-mingw-debug --target double_click_hotkey` completed successfully
  (`ninja: no work to do`), confirming the configured Windows cross-build target is current.
- Individual test cases, fixture data, test logic, assertions, and coverage adequacy are not reviewed, per the review
  scope and skill guardrails.
- Windows platform implementation details are outside this segment and are inspected only where needed to validate the
  portable contract.
- No runtime validation was performed on Windows; the Windows check was a Linux-to-Windows MinGW-w64 cross-build only.
