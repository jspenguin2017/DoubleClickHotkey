# Usage details

See the [README](../README.md#usage) for launch options and [startup setup](../README.md#run-at-startup).

## Console and commands

Console hiding requires Windows Console Host as the default terminal application. Select it under
`Settings > System > Advanced > Terminal`.

Each interactive Windows session can run its own service. `--show`, `--hide`, and `--send-f13` are one-shot commands
that keep their own console visible. Show/hide commands report failure if no ready instance is available, including
during startup; use `--start-shown` when the service console must be visible from the beginning.

Double-click injection errors are logged without revealing a hidden console. Use `--show` to inspect them.

## Elevation and security

[Windows restricts synthesized input](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput)
to applications at an equal or lower integrity level. To double-click or send F13 to an elevated application, run Double
Click Hotkey at the same or a higher integrity level. Windows does not identify when this restriction caused an
injection failure; failures without an error code list an integrity-level mismatch as one possible cause.

Showing and hiding the console are not treated as sensitive operations. Within the same interactive session, a
non-elevated launch may show or hide an elevated service's console. The command channel accepts only `--show` and
`--hide`; it cannot request F13 or mouse input injection through the service.

The threat model covers interference from ordinary webpages in a secure, uncompromised browser. Browser extensions and
denial of service by native processes in the same interactive session are outside its scope.
