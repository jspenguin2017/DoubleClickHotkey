# Code Review: Windows Platform

## Reviewed scope and basis

- Scope: `src/platform/windows/` and the public platform contracts it implements in
  `include/double_click_hotkey/platform_binding.hpp` and `include/double_click_hotkey/platform_factory.hpp`.
- Review basis: repository state at commit `9e03888e53b069b9f0dd2ffdeef8ba97c9774daf` on 2026-08-30.
- Review method: traced native resource ownership, startup/shutdown, console-control delivery, single-instance state,
  cross-process show/hide commands, low-level keyboard-hook dispatch, message/event multiplexing, and synthesized
  keyboard and mouse input. Win32 semantics were checked against the linked Microsoft API documentation.

## Findings

### W-4: UIPI-blocked input can be reported as error code zero

- Severity: **Low**
- References: `src/platform/windows/input_injector.cpp:13-29` and
  `src/platform/windows/windows_platform_binding.cpp:111-150`.
- Problem: The input injector clears the thread's last-error value before calling `SendInput`, then propagates
  `GetLastError()` whenever the inserted count is short. Windows explicitly states that neither the return value nor
  `GetLastError` identifies a failure caused by User Interface Privilege Isolation (UIPI). Consequently, an attempt to
  target an elevated application can leave the cleared value unchanged and be logged only as `error code: 0`, which
  reads like success rather than a useful failure. See the
  [Microsoft `SendInput` UIPI documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).
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
