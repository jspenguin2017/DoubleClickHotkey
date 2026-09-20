# Usage details

See the [README](../README.md#usage) for basic controls and [startup setup](../README.md#run-at-startup).

## Tray and window

The application starts hidden without opening a console. Click the tray icon or choose **Show** to restore its window;
**Quit** exits. Closing the window only hides it. A second launch shows the existing instance, including across
administrator elevation levels within the same interactive session. Windows may still restrict foreground activation.
Duplicate launches wait up to two seconds for startup to finish before reporting a failure.

The window can be resized and scales with display DPI. The app restores its tray icon after Explorer restarts. If the
icon cannot be created or restored, it shows the window and logs an error. **Quit Double Click Hotkey** in the window's
system menu remains available; closing the window while the tray is unavailable keeps it visible.

Command-line arguments are ignored, including the former `--start-shown`, `--show`, `--hide`, and `--send-f13` options.
Close any older console version before starting this version; the two versions share instance ownership but use
incompatible activation channels.

## Log

The selectable, read-only log retains the newest 500 newline-delimited lines in memory, including blank lines. Oldest
lines are removed as new output arrives. Long lines scroll horizontally rather than wrapping. The log follows new output
when at the bottom with no selection; otherwise it preserves the selection and reading position where the retained text
permits. Use the usual copy shortcut or **Ctrl+A** to select all.

Input errors continue to be logged while hidden without showing the window. Unrecoverable startup or event-loop errors
are reported in a dialog before exit. No log file is written; quitting clears the history.

## Sending F13

**Delay (seconds)** accepts whole numbers from 1 through 3600. Invalid or empty input disables **Send F13**. The default
is five seconds on each launch, and edits last until the app quits.

Click **Send F13**, then focus the target program. The delay field and button are disabled during the countdown; the
button shows the remaining seconds. Sending logs its completion or error and re-enables the controls without moving
focus. Only one send can be pending.

Closing or hiding the window leaves the countdown running. Sleep and hibernation count toward the delay; an overdue send
occurs after resume. Changes to the system clock do not alter the delay. Quitting cancels a pending send.

The app passes its own generated F13 press and release through its hook so another program can capture them. Other F13
input continues to trigger one double-click per press, including during countdowns. No instance message can request F13
injection, double-click injection, or Quit.

## Elevation and security

[Windows restricts synthesized input](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput)
to applications at an equal or lower integrity level. To double-click or send F13 to an elevated application, run Double
Click Hotkey at the same or a higher integrity level. Windows does not identify when this restriction caused an
injection failure; failures without an error code list an integrity-level mismatch as one possible cause.

Showing the window is not treated as a sensitive operation. The session-local Show event grants only the rights needed
for cross-elevation activation and cannot request any other operation. Tags on this app's generated keystrokes prevent
self-interception; they are not an authentication mechanism.

The threat model covers interference from ordinary webpages in a secure, uncompromised browser. Browser extensions and
denial of service by native processes in the same interactive session are outside its scope.
