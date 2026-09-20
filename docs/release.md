# Release size and validation

See [development](development.md#direct-cmake) for build commands and [AGENTS.md](../AGENTS.md) for contribution
constraints.

## Optimization and distribution

Ship one Windows 11 x64 executable with only Windows-provided DLL dependencies. Preserve:

- Static linking (`-static -static-libgcc -static-libstdc++`), C++ exceptions, and RTTI.
- MinGW `Release`/`MinSizeRel` size optimization across all application targets: `-Os` at compile and link time,
  function/data sections, LTO when supported, unused-section removal, and symbol stripping. CMake warns if LTO is
  unavailable; `Debug` and `RelWithDebInfo` must retain debugging information.
- Stream-free integer formatting for error codes and countdowns; diagnostics go through the portable log and native
  read-only control. Avoid C++ stream and locale machinery.

## Validation

For changes that can affect binary size (application code, build/link settings, dependencies, or resources):

1. Save a baseline Release executable, then rebuild with the same compiler, runtime libraries, architecture, and
   configuration. Report before/after bytes and percentage change; investigate unexpected growth and fix regressions or
   explain necessary increases.
2. Inspect DLL imports: no `libgcc`, `libstdc++`, or `libwinpthread` dependencies. Windows-provided `msvcrt.dll` and
   UCRT API sets are acceptable. Static-link flags alone are insufficient.
3. Verify the Windows GUI subsystem, embedded icon/version/Per-Monitor V2 manifest, and ASLR/NX (`DYNAMIC_BASE` and
   `NX_COMPAT`). For build-setting changes, also check `Debug` and `RelWithDebInfo` debugging information.

Measure and inspect a Linux cross-build with:

```sh
wc -c < build/linux-mingw-release/DoubleClickHotkey.exe
x86_64-w64-mingw32-objdump -p build/linux-mingw-release/DoubleClickHotkey.exe
```

On Windows, measure with PowerShell `(Get-Item build/windows-mingw-release/DoubleClickHotkey.exe).Length` and use the
toolchain's `objdump -p` with that path.

Manually verify affected Windows behavior: F13 handling, tray/duplicate activation (including cross-elevation), scaled
icons, log selection/scrolling, DPI/resizing, countdowns across sleep, and shutdown. Report unavailable checks as
pending; cross-compilation and binary inspection do not verify runtime behavior.

## Size history

Recorded through 2026-09-19 using Linux x64 cross-builds with MinGW-w64 GCC 13-win32 and `linux-mingw-release`. Each row
uses its own baseline; these are reference measurements, not limits across toolchains.

| Change                                | Before (bytes) | After (bytes) |  Change |
| ------------------------------------- | -------------: | ------------: | ------: |
| Size flags and stream-free formatting |      2,506,211 |       178,688 | −92.87% |
| Native tray application               |        178,688 |       241,152 | +34.96% |
| Single PNG-compressed 256×256 icon    |        240,640 |       232,960 |  −3.19% |

The tray increase added the native window/tray, Unicode log, bounded log buffer, and nonblocking countdown. Recorded
builds retained Windows-only DLL imports and ASLR/NX; native Windows desktop and scaled-icon checks remain pending.
