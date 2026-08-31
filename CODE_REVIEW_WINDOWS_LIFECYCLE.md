# Code Review: Windows Lifecycle and IPC

## Reviewed scope and basis

- Scope: `src/platform/windows/console.*`, `console_control_handler.*`, `instance_command.*`,
  `named_object_security.hpp`, `single_instance.*`, `windows_platform_binding.*`, and `platform_factory.cpp`.
- Basis: the clean worktree at commit `ab20ddd45599`, reviewed as three logical segments: console/control handling,
  single-instance command IPC/security, and platform-binding lifecycle integration.
- Related contracts inspected for validation: `include/double_click_hotkey/platform_binding.hpp`,
  `include/double_click_hotkey/application.hpp`, `src/application/application.cpp`, the keyboard-hook interface and
  implementation, `README.md`, and relevant build configuration.
- Third-party, generated, and vendored code is excluded. Individual test cases and assertions are not reviewed.

## Findings

### 1. A session process can pre-create the fixed named objects and block service startup

- Severity: Medium
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

### 2. Sustained command signaling can starve the service's message pump

- Severity: Medium
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

## Segment status

- Console and console-control handling: reviewed. No findings were identified; this does not imply that the code is
  defect-free.
- Single-instance command IPC/security: reviewed. Finding 1 applies.
- Platform-binding lifecycle integration: reviewed. Finding 2 applies. No findings were identified in
  `platform_factory.cpp`; this does not imply that the file is defect-free.

## Unresolved questions

- The repository does not state whether malicious processes in the same interactive session are outside its threat
  model. Findings 1-2 treat the documented permission to request non-sensitive visibility changes as distinct from
  permission to impersonate or deny service to the running instance.

## Checks and areas not covered

- Worktree cleanliness was verified before this report was created.
- The apparent missing flush before `_getch()` in `Console::WaitForKey` was checked against the configured MinGW-w64
  runtime and not retained as a finding: default `std::cout` is synchronized to C stdio, the relevant libstdc++ stream
  buffer delegates the write to `fwrite`, and the Windows CRT flushes standard output after each library call when it is
  attached to a character device.
- Windows runtime behavior has not yet been exercised; review is running on a Linux host.
- `cmake --build --preset linux-mingw-debug` and `cmake --build --preset linux-mingw-release` completed successfully,
  compiling and linking the reviewed Windows sources with the repository's configured MinGW-w64 toolchain.
- Relevant semantics were checked against Microsoft documentation for named-object collisions and mutex pre-creation,
  event behavior, `MsgWaitForMultipleObjectsEx`, console control handlers, and `LowLevelKeyboardProc` message-loop and
  timeout requirements.
- The cross-compiled Windows executable was not run. Named-object pre-creation and command-flood scenarios were not
  reproduced dynamically on Windows; their findings are based on the current access masks and documented Win32 behavior.
