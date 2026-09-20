# Windows desktop code review

## Scope and basis

- Repository-wide review segment: `src/platform/windows/main_window.*`, `tray_icon.*`, `windows_platform_binding.*`,
  `native.*`, `platform_factory.cpp`, and the native manifest and resource integration, with the controller and usage
  documentation as related contracts.
- Basis: commit `c502829e0f39009868ec79818186779d42e6bb6b`; initially clean worktree.
- Reviewed startup/teardown, native callback exception containment, timer integration, tray recovery, control
  presentation, keyboard navigation, log selection/scroll restoration, DPI changes, visibility, and session shutdown.

## Findings

### 1. Preserve keyboard focus before disabling the Send control

**Severity:** Medium

**Location:** `src/platform/windows/main_window.cpp:297-298`. Related paths: `src/application/application.cpp:133-138`
and `src/platform/windows/main_window.cpp:308-323,427-434`.

**Problem:** Starting a countdown disables both controls without moving focus away from the focused control. In the
normal click or keyboard activation path, Send F13 has focus when `EnableWindow(send_, false)` runs. Windows clears
keyboard focus when the focused window is disabled. Subsequent presentations re-enable the controls but never assign
focus again; `RestoreFocus()` only runs when showing or refocusing the top-level window.
[Microsoft's window focus contract](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features).

**Impact:** A user who starts a countdown and stays in this window is left without a focused control, including after
completion. Ordinary control keyboard input loses its target until the user clicks a control or reactivates the window.
The custom Ctrl+A/Enter handling only handles `WM_KEYDOWN` addressed to the individual controls; with no focus, Windows
directs key presses to the active window as `WM_SYSKEYDOWN` instead.
[Microsoft's WM_SYSKEYDOWN contract](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-syskeydown).

**Recommendation:** Before disabling a currently focused delay/send control, move focus to an enabled control such as
the log. Only do this when one of those controls currently owns focus, and do not activate the app or change another
application's focus on countdown completion.

**Verification:** Traced the normal button activation, controller state update, presentation, and completion paths
against the documented Win32 behavior. This is a static/API-contract finding; an interactive Windows reproduction was
not available.

### 2. Return focus to the notification area when its menu is canceled

**Severity:** Low

**Location:** `src/platform/windows/tray_icon.cpp:71-84`.

**Problem:** Opening the tray menu foregrounds the application's window. When the menu is canceled, the code posts
`WM_NULL` and returns without calling `Shell_NotifyIconW(NIM_SETFOCUS, ...)`. That posted message does not restore
notification-area focus. Microsoft specifically requires this focus handoff after canceling a notification icon menu
with Escape.
[Microsoft's NIM_SETFOCUS contract](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw).

**Impact:** Keyboard users who navigate to the tray icon, open its menu, and press Escape lose the expected
notification-area focus and cannot seamlessly continue navigating its icons. The hidden application window can remain
the foreground owner.

**Recommendation:** On menu cancellation, return focus with `NIM_SETFOCUS` while the tray icon remains available.
Preserve the existing Show and Quit focus behavior.

**Verification:** Checked every tray-menu return path and searched the current repository for another focus handoff.
None exists. Interactive Windows verification remains pending.

## Unresolved questions

- Confirm tray click/menu delivery and Explorer-restart recovery when the application is elevated. The code explicitly
  permits `TaskbarCreated`; this host cannot validate the shell's delivery of the regular tray callback across integrity
  levels. This is not reported as a verified elevation defect.

## Checks and limitations

- Traced teardown through immediate quit, native callback failure, startup failure, and normal event-loop exit. Checked
  callback detachment and resource ownership order.
- Reviewed CRLF conversion, retained-line offsets, selection and scroll restoration, DPI resource replacement,
  hidden-window behavior, and tray-unavailable fallback.
- The version-zero tray notification choice is explicitly documented in the implementation; its use alone is not
  reported as a defect.
- Windows execution is unavailable. DPI/multiple-monitor behavior, assistive technology, log-control interaction,
  Explorer restart, native modal loops, and session-end timing were not tested interactively. Passing portable tests
  does not validate these behaviors.
- No production code, tests, configuration, or resources were edited.
