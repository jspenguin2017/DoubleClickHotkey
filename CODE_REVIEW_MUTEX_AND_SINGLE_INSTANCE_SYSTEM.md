# Code Review: Mutex and Single-Instance System

## Scope and review basis

This review covers the Windows mutex-backed single-instance reservation and its directly coupled instance-command
channel: `src/platform/windows/single_instance.*`, `src/platform/windows/instance_command.*`, the relevant paths in
`src/platform/windows/windows_platform_binding.*`, the portable application contract and orchestration that invoke those
paths, and the related user-facing behavior in `README.md`. The review is based on clean commit
`ba751c4c96f51e91a5c7a966c0e00925c0eefa32`.

Windows behavior was checked against Microsoft's documentation for
[`CreateMutexW`](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createmutexw),
[`CreateMutexExW`](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createmutexexw),
[`CreateFileMappingW`](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingw),
[`CreateEventW`](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventw),
[`Synchronization Object Security and Access Rights`](https://learn.microsoft.com/en-us/windows/win32/sync/synchronization-object-security-and-access-rights),
[`Object Namespaces`](https://learn.microsoft.com/en-us/windows/win32/sync/object-namespaces), and
[`Mandatory Integrity Control`](https://learn.microsoft.com/en-us/windows/win32/secauthz/mandatory-integrity-control).

## Findings

### 1. A normal-integrity launch cannot recognize or manage an elevated service

- **Severity:** Medium
- **References:** `src/platform/windows/single_instance.cpp:8`, `src/platform/windows/instance_command.cpp:146-153`,
  `src/platform/windows/windows_platform_binding.cpp:62-65`, `src/platform/windows/windows_platform_binding.cpp:77-83`,
  `README.md:22-26`
- **Problem:** The documented way to control an elevated target is to run Double Click Hotkey at the same or a higher
  integrity level. Objects created by that process therefore receive the elevated integrity level. A later
  normal-integrity process cannot obtain write access to them. In particular, `CreateMutexW` requests `MUTEX_ALL_ACCESS`
  when the named mutex already exists, while the command sender requests `EVENT_MODIFY_STATE` and `FILE_MAP_WRITE`. The
  mutex request fails with access denied and is classified as a native setup failure instead of `already_running`;
  `--show` and `--hide` likewise fail rather than reaching the elevated service. The README discusses the integrity
  requirement for synthesized input, but not for these management operations.
- **Impact:** After following the elevation guidance, a user cannot reveal the hidden service console from an ordinary
  terminal, and an accidental ordinary relaunch reports a misleading mutex-creation failure rather than identifying the
  existing instance. `--send-f13` is similarly unable to distinguish an elevated service from a mutex setup error.
- **Recommendation:** Make the reservation existence-based instead of ownership-based and use `CreateMutexExW` with no
  initial ownership and only the non-mutating access needed to retain/recognize the object. Separately choose and
  document the command channel's integrity policy: either translate access denied into a clear instruction to rerun the
  command elevated, or, after assessing the lower-to-higher trust boundary, give only the same interactive user the
  minimum event and mapping rights needed for these two fixed commands.

### 2. The named-object protocol trusts objects pre-created by another process

- **Severity:** Medium
- **References:** `src/platform/windows/windows_platform_binding.cpp:18`,
  `src/platform/windows/single_instance.cpp:8-17`, `src/platform/windows/instance_command.hpp:23-26`,
  `src/platform/windows/instance_command.cpp:84-102`, `src/platform/windows/windows_platform_binding.cpp:194-215`
- **Problem:** The mutex, command mapping, and event all use fixed public names. Microsoft explicitly warns that a
  predictable single-instance mutex can be created first by another process to prevent the application from starting;
  this code accepts that object as proof that another legitimate instance is running. The receiver also never
  distinguishes a newly created mapping or event from an existing one. A normal-integrity process in the same session
  can create a writable mapping and a manual-reset event under those names, retain handles, and then let the service
  start elevated under the same account. The elevated process is allowed to open the lower-integrity objects. Windows
  documents that `CreateFileMappingW` returns the existing mapping with its existing size and that `CreateEventW`
  ignores the requested reset mode when it opens an existing event, setting `ERROR_ALREADY_EXISTS` in both cases. This
  code ignores that result, zeroes the attacker's mapping, and treats the attacker's event as the auto-reset event it
  requested.
- **Impact:** The lower-integrity process retains write access to the elevated service's command state and can inject
  show/hide commands. More seriously, it can signal a pre-created manual-reset event once; because the receiver never
  calls `ResetEvent`, `MsgWaitForMultipleObjectsEx` then returns that handle continuously and the loop's event branch
  immediately continues. The elevated service can spin a CPU core while starving its Windows message processing and
  hotkey behavior. Independently, pre-creating and retaining the mutex prevents both the service and `--send-f13` mode
  from starting for as long as the foreign handle remains open. A sender also accepts a foreign event/mapping pair and
  can report command success when no service exists.
- **Recommendation:** Treat `ERROR_ALREADY_EXISTS` from either receiver-side creation call as a violated ownership
  invariant, close every partial resource, and fail initialization. Protect against the remaining pre-creation denial of
  service by placing all three related objects in a private namespace restricted to the intended user/session, or by
  using an unguessable per-user channel identity stored in a location that lower-integrity callers cannot replace. Give
  the resulting objects explicit, minimum-rights security descriptors consistent with the chosen command policy.

### 3. Commands can report that no instance is running while the service is starting

- **Severity:** Low
- **References:** `src/platform/windows/windows_platform_binding.cpp:29-41`,
  `src/platform/windows/instance_command.cpp:146-149`, `src/platform/windows/windows_platform_binding.cpp:77-80`
- **Problem:** `RunService()` publishes the single-instance mutex before `InstanceCommandReceiver::Initialize()` creates
  the event that acts as the sender's readiness gate. A `--show` or `--hide` process scheduled in that interval sees
  `ERROR_FILE_NOT_FOUND` from `OpenEventW`, and `SendWindowCommand()` immediately translates it to `not_running`. There
  is no readiness handshake, mutex probe, or bounded retry to distinguish startup from absence.
- **Impact:** A script or shortcut that starts the service and immediately asks it to show or hide can fail
  nondeterministically with "No running instance ... was found," even though the service process owns the singleton and
  is still initializing. Retrying manually works, making the failure intermittent and difficult to diagnose.
- **Recommendation:** Treat the command event as a readiness publication and, when it is absent, use a minimum-rights
  singleton probe plus a short bounded wait/retry for the event. Stop retrying if the reservation disappears or the
  timeout expires. Keep mapping creation before event creation so an openable event continues to mean that the whole
  channel is ready.

## Unresolved questions

- Should a normal-integrity `--show` or `--hide` command be permitted to control a service elevated under the same user,
  or is matching the service's integrity level an intentional security requirement? The implementation and README do not
  currently establish that policy.
- Is the singleton intended to be per interactive session or system-wide? Because the names have neither a `Global\`
  prefix nor a private global namespace, Windows gives the current implementation one independent instance per session.

## Checks and areas not covered

- Traced the service, `--show`, `--hide`, and `--send-f13` call paths through the portable application and Windows
  binding.
- Checked the named-object access requests and default-security behavior against the Microsoft documentation linked
  above.
- Searched the repository for every creator/opener of the three named objects and for security descriptors, private
  namespaces, access-denied handling, readiness probes, and reset operations; no alternate ownership or readiness path
  exists.
- No Windows runtime check was run; this Linux host cannot exercise Windows integrity levels or named-kernel-object
  behavior natively. The MinGW cross-build would validate compilation but not the reviewed runtime semantics.
- Individual unit-test cases and coverage adequacy are outside the review skill's permitted test scope.
