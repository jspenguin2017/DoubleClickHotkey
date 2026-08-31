# Code Review: Windows Lifecycle and IPC

## Reviewed scope and basis

- Scope: `src/platform/windows/console.*`, `console_control_handler.*`, `instance_command.*`,
  `named_object_security.hpp`, `single_instance.*`, `windows_platform_binding.*`, and `platform_factory.cpp`.
- Basis: the clean worktree at commit `ab20ddd45599`, reviewed as three logical segments: console/control handling,
  single-instance command IPC/security, and platform-binding lifecycle integration.
- The findings were dispositioned against the implementation at commit `950414da62df` after the threat model was
  clarified. The intervening runtime changes affect F13 transition handling and swapped mouse buttons, not the lifecycle
  or IPC conditions described below.
- Related contracts inspected for validation: `include/double_click_hotkey/platform_binding.hpp`,
  `include/double_click_hotkey/application.hpp`, `src/application/application.cpp`, the keyboard-hook interface and
  implementation, `README.md`, and relevant build configuration.
- Third-party, generated, and vendored code is excluded. Individual test cases and assertions are not reviewed.

## Threat model and result

The in-scope attacker is an ordinary webpage running in a secure, uncompromised browser, not a malicious browser
extension. Denial of service by a native process in the same interactive Windows session is explicitly out of scope.
Browser compromise or sandbox escape and separately installed or downloaded native code are not webpage-only attacks.

The reviewed implementation meets this threat model. It exposes no network listener, registered URL protocol, file
association, browser native-messaging integration, or web-content parser. A webpage has no web-platform API for opening
the utility's Win32 named objects, signaling its event, posting to its native thread queue, or inserting input into the
Windows input stream. Script-created
[DOM events are initialized as untrusted](https://dom.spec.whatwg.org/#dom-event-istrusted) and remain inside the
browser; the utility responds only to F13 input delivered by Windows to its
[`WH_KEYBOARD_LL` hook](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc).

The named command channel does not create an input-injection deputy: every encoded state decodes to one of the two fixed
console-visibility operations, `show_window` or `hide_window`. Neither command reaches `SendF13` or `DoubleClick`.
Showing and hiding the console are intentionally non-sensitive. A user pressing F13 while a webpage is active does
intentionally send the resulting double-click to the current pointer location; that is the utility's requested behavior,
not a page-originated trigger.

## Findings and dispositions

### 1. A session process can pre-create the fixed named objects and block service startup — accepted

- Original severity: Medium
- Disposition: Outside the threat model because the impact is denial of service by a native process in the same
  interactive session.
- Files: `src/platform/windows/windows_platform_binding.cpp:18,29-44,57-65`;
  `src/platform/windows/single_instance.cpp:24-35`; `src/platform/windows/instance_command.hpp:23-26`;
  `src/platform/windows/instance_command.cpp:100-117,140-155`
- Problem: the mutex, command-state mapping, and command event all use public, compile-time names. `CreateMutexExW`
  opens an existing mutex of that name and reports `ERROR_ALREADY_EXISTS`; the code then trusts mere name existence as
  proof that another valid instance is running. If the mutex is free, `InstanceCommandReceiver::Initialize` explicitly
  rejects either command object when its creation reports `ERROR_ALREADY_EXISTS`. The supplied security descriptors do
  not authenticate objects that already exist. A process in the same session can therefore pre-create the mutex to force
  `already_running`, or leave the mutex free and pre-create either IPC object to make receiver initialization fail. A
  restrictive DACL can instead make an open fail, and reserving any name with another shared-namespace kernel-object
  type also makes creation fail. The
  [Win32 object-name rules](https://learn.microsoft.com/en-us/windows/win32/sync/object-names) document these
  existing-object and cross-type collisions, and Microsoft
  [explicitly warns about the pre-creation attack](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createmutexexw)
  when a fixed named mutex is used as a single-instance guard.
- Impact: an untrusted session process can prevent the service and the five-second `--send-f13` workflow from starting
  for as long as it retains a spoofed mutex; pre-creating the mapping or event prevents service startup while leaving
  the one-shot reservation path available. This lets a non-elevated process block a later elevated service launch and
  can mislead the user into looking for an instance that does not exist or into diagnosing a native initialization
  failure.
- Recommendation: secure the reservation and both IPC objects as one authenticated instance channel. For example, use a
  non-pre-creatable reservation such as an appropriately ACL-protected, session-specific locked file, generate random
  IPC object names for the owning instance, and publish them through ACL-protected state. If fixed named rendezvous
  objects remain necessary, validate the owning instance rather than treating any of the three names as identity.
  Preserve the documented per-session behavior when choosing the protected storage scope.
- Threat-model analysis: exploiting this condition requires creating or opening Windows kernel objects from native code.
  The application provides no browser-facing bridge to those APIs. A secure browser does not turn webpage script into a
  same-session native process, so the condition is not reachable by the in-scope attacker. The recommendation remains
  applicable if same-session native-process availability becomes a security objective.

### 2. Sustained command signaling can starve the service's message pump — accepted

- Original severity: Medium
- Disposition: Outside the threat model because the impact is denial of service by a native process in the same
  interactive session.
- Files: `src/platform/windows/windows_platform_binding.cpp:203-214,221-231,243-261`;
  `src/platform/windows/instance_command.cpp:15,196-228`; `src/platform/windows/console_control_handler.cpp:45-53`
- Problem: after the command event satisfies `MsgWaitForMultipleObjectsEx`, the loop handles at most one latest command
  and immediately `continue`s. It does not inspect or drain the message queue on that path. Interactive clients are
  intentionally allowed to call `SetEvent`, so a process that signals the
  [auto-reset event](https://learn.microsoft.com/en-us/windows/win32/sync/event-objects) continuously can keep it ready
  across successive waits. The documented
  [`MsgWaitForMultipleObjectsEx` contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex)
  returns either a ready handle or queued input; the surrounding loop contains no fairness mechanism that guarantees
  queued messages are serviced between command-event results.
- Impact: keyboard-hook delivery and the posted semantic hotkey events both depend on this thread pumping messages, and
  the console handler also posts `WM_QUIT` to the same queue. Sustained signaling can therefore stop F13 handling and
  prevent Ctrl+C/Ctrl+Break from shutting down the service. Microsoft documents that
  [`WH_KEYBOARD_LL` invokes the installing thread through its message loop and is subject to silent timeout removal](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc).
  This turns the deliberately non-sensitive visibility channel into a way for a normal session process to disrupt the
  utility's core behavior, including when the receiver is elevated.
- Recommendation: service queued messages after every command-event result instead of unconditionally continuing, with
  bounded/fair handling when both sources remain ready and prompt priority for `WM_QUIT`. A dedicated wait thread that
  posts a private message to the hook-owning thread is another way to keep the hook message pump continuously
  responsive.
- Threat-model analysis: sustaining the condition requires a native caller to obtain `EVENT_MODIFY_STATE` and repeatedly
  call `SetEvent`. A webpage cannot obtain that handle or invoke `SetEvent` through the web platform, and the
  application exposes no browser integration that would do so on its behalf. The recommendation remains a valid
  robustness measure if same-session native-process availability enters scope.

## Segment status

- No unresolved findings apply to the clarified webpage-origin threat model.
- Console and console-control handling: reviewed; no in-scope findings were identified.
- Single-instance command IPC/security: reviewed; finding 1 is retained as an explicitly accepted availability
  limitation.
- Platform-binding lifecycle integration: reviewed; finding 2 is retained as an explicitly accepted availability
  limitation. No findings were identified in `platform_factory.cpp`.

## Unresolved questions

None. The same-session denial-of-service policy and webpage-origin security boundary are now documented in `README.md`.

## Checks and areas not covered

- Worktree cleanliness was verified before this report was created.
- The apparent missing flush before `_getch()` in `Console::WaitForKey` was checked against the configured MinGW-w64
  runtime and not retained as a finding: default `std::cout` is synchronized to C stdio, the relevant libstdc++ stream
  buffer delegates the write to `fwrite`, and the Windows CRT flushes standard output after each library call when it is
  attached to a character device.
- Windows runtime behavior has not yet been exercised; review is running on a Linux host.
- Post-disposition validation completed successfully: `npm run test` passed all 30 native tests;
  `cmake --build --preset linux-mingw-debug` and `cmake --build --preset linux-mingw-release` compiled and linked the
  Windows application with MinGW-w64; and `npm run format:check` passed.
- Relevant semantics were checked against Microsoft documentation for named-object collisions and mutex pre-creation,
  event behavior, `MsgWaitForMultipleObjectsEx`, console control handlers, and `LowLevelKeyboardProc` message-loop and
  timeout requirements.
- The disposition pass traced every externally reachable input in the current source. Process arguments are parsed only
  after native process launch; instance IPC decodes only fixed show/hide commands; the keyboard hook accepts only
  Windows F13 input; and no source registers or listens on a browser-reachable interface.
- The generated Debug and Release PE import tables contain only `ADVAPI32.dll`, `KERNEL32.dll`, `msvcrt.dll`, and
  `USER32.dll`, consistent with the statically reviewed native surface. Both executables have high-entropy ASLR, dynamic
  base, and NX compatibility enabled.
- The cross-compiled Windows executable was not run. Named-object pre-creation and command-flood scenarios were not
  reproduced dynamically on Windows; their findings are based on the current access masks and documented Win32 behavior.
