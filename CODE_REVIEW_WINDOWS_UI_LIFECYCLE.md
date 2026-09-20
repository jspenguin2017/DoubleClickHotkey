# Windows UI and lifecycle review

## Scope and basis

- Repository-wide review, UI/lifecycle segment: `src/platform/windows/main_window.*`, `tray_icon.*`,
  `single_instance.*`, `named_object_security.hpp`, `native.*`, `windows_platform_binding.*`, `platform_factory.cpp`,
  and `clock.*`.
- Basis: clean starting worktree at commit `53a29b2de15af1c7ee7d81b84530588a066c7a54`, repository instructions, and
  README/usage/development contracts.
- Traced hidden startup, duplicate activation, named-object lifetime and access masks, the event loop, modal callbacks,
  countdown timers, exception containment, resource ownership, shutdown, tray recovery, focus changes, DPI/layout,
  Unicode conversion, and log selection/scroll retention.

## Findings

No verified findings in this segment. This does not establish that the code is defect-free.

## Unresolved questions

None established as an actionable defect through static review. Runtime behavior listed below remains unverified.

## Checks and limitations

- Checked wait-loop wakeup behavior against
  [MsgWaitForMultipleObjectsEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex),
  event creation/reset semantics against
  [CreateEventExW](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventexw), and focus
  operations against [SetFocus](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setfocus).
- Checked legacy tray notification routing and menu cancellation against
  [Shell_NotifyIconW](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw). The
  explicit choice of legacy notification routing was considered rather than treated as a defect merely for not using
  version 4.
- The existing Debug and Release Windows executables are current according to `ninja -n` for their application targets.
  They were not executed on this Linux host.
- Interactive Windows 11 validation was unavailable: Explorer restart, cross-elevation activation, rapid duplicate
  launch/quit, modal shutdown, keyboard and screen-reader navigation, log selection/scrolling, display/DPI changes, and
  countdowns across sleep still require native checks.
- The documented exclusion of same-session native-process denial of service was respected when assessing the named
  objects and native messages.
- No source or configuration changes were made.

## Review status

Complete.
