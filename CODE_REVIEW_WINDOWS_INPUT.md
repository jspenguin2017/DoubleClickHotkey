# Code Review: Windows Input

## Reviewed scope and basis

- Scope: `src/platform/windows/input_injector.*`, `src/platform/windows/keyboard_hook.*`,
  `src/platform/windows/keyboard_sender.*`, and `src/platform/windows/mouse.*`, plus the directly related contracts and
  call sites in `include/double_click_hotkey/platform_binding.hpp`, `src/application/application.cpp`, and
  `src/platform/windows/windows_platform_binding.*`.
- Review basis: repository state at commit `ab20ddd45599c92ed310d624d597a2b22d8fed18` on 2026-08-31. Third-party,
  generated, and vendored code is excluded.
- Review method: static tracing of low-level keyboard-hook installation and teardown, F13 transition suppression and
  queuing, message-loop delivery, synthesized F13 and primary-button sequences, partial-injection recovery, and error
  propagation into the portable application controller.

## Findings

### WI-2: The double-click does not honor a swapped primary mouse button

- Severity: **Medium**
- References: `src/platform/windows/mouse.cpp:21-24`, `src/platform/windows/windows_platform_binding.cpp:158`, and
  `README.md:3-8`.
- Problem: `Mouse::DoubleClick` always emits `MOUSEEVENTF_LEFTDOWN`/`MOUSEEVENTF_LEFTUP`, even though Windows exposes
  `SM_SWAPBUTTON` specifically because the meanings of the left and right buttons can be swapped. `SendInput` inserts
  its records into the mouse input stream, and Microsoft documents that a swapped configuration makes the left button
  generate right-button messages and vice versa. Two independent production `SendInput` implementations confirm the
  consequence for injected flags: Chromium swaps left/right flags before injection explicitly “to un-do that
  re-mapping,” and OpenJDK's Windows Robot emits `MOUSEEVENTF_RIGHT*` for logical button 1 when `SM_SWAPBUTTON` is set.
  Thus `MOUSEEVENTF_LEFT*` is not an unconditional logical `WM_LBUTTON*` action; it is remapped to the secondary action
  in a swapped configuration. The binding calls the affected button the “primary mouse button,” but the implementation
  never performs the required inverse mapping. Evidence:
  [SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput),
  [SwapMouseButton](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-swapmousebutton),
  [Chromium Windows input injector](https://chromium.googlesource.com/chromium/src/+/master/remoting/host/input_injector_win.cc#113),
  and
  [OpenJDK Windows Robot](https://github.com/openjdk/jdk/blob/master/src/java.desktop/windows/native/libawt/windows/awt_Robot.cpp#L1045-L1110).
- Impact: For users who swap mouse buttons, including left-handed users, F13 can synthesize the secondary-button action
  (commonly a context menu) rather than the primary double-click that activates or opens the target under the pointer.
  The utility's core workflow is therefore broken under a standard Windows accessibility/input configuration.
- Recommendation: Query `GetSystemMetrics(SM_SWAPBUTTON)` when constructing the sequence and use right-button down/up
  flags when the buttons are swapped, otherwise left-button flags. Keep the adjacent down/up pair structure so partial
  injection recovery remains valid.

## Segment status

### Keyboard hook and queued event delivery

Finding WI-1 was identified in this segment. Apart from that finding, no additional findings were identified; this does
not imply the segment is defect-free.

### Synthesized input and error propagation

Finding WI-2 was identified in mouse-button selection. No findings were identified in balanced partial-send recovery,
F13 sequence construction, or injection-error propagation; this does not imply those areas are defect-free.

## Unresolved questions

None.

## Checks and areas not covered

- Statically traced hook callbacks through `WindowsPlatformBinding::RunMessageLoop` and
  `Application::HandleHotkeyEvent`.
- Statically traced `KeyboardSender` and `Mouse` through `InputInjector`, including every possible returned prefix count
  for the current two- and four-record balanced sequences, compensating-release behavior, and platform error messages.
- `x86_64-w64-mingw32-g++ -std=c++17 -Wall -Wextra -Wpedantic -DNOMINMAX -DUNICODE -DWIN32_LEAN_AND_MEAN -D_UNICODE -Iinclude -Isrc -fsyntax-only`
  completed successfully for all reviewed Windows input translation units and their `WindowsPlatformBinding` call site.
- `cmake --build --preset linux-mingw-debug --target double_click_hotkey_platform` completed successfully; the existing
  Ninja build was already current and performed no compilation.
- Checked the relevant behavior against Microsoft's `LowLevelKeyboardProc`, keyboard-input, `SendInput`, `MOUSEINPUT`,
  `SwapMouseButton`, and system-metrics documentation. For the otherwise implicit `SendInput`/`SM_SWAPBUTTON`
  interaction, cross-checked Chromium's Windows input injector and OpenJDK's Windows Robot; both invert injected
  left/right flags when buttons are swapped, and Chromium states that this undoes the operating-system remapping.
- Windows-only runtime behavior could not be exercised on the Linux review host. Hook installation and teardown,
  pre-held/repeated F13 delivery, actual synthesized input, UIPI failure reporting, partial native sends, and swapped
  mouse-button behavior remain dynamically unverified.
- Individual test cases, fixtures, assertions, and coverage adequacy were not reviewed.
