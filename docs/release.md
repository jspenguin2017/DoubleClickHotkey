# Release size and validation

See [development](development.md#direct-cmake) for build commands and [AGENTS.md](../AGENTS.md) for contribution
constraints.

## Optimization and distribution

Ship a single Windows 11 x64 executable with no separately installed runtime. MinGW `Release` and `MinSizeRel` optimize
the core, adapter, and executable with `-Os`, function/data sections, and checked link-time optimization (LTO). Match
compile and link optimization levels; CMake warns if LTO is unavailable. The linker removes unused sections and strips
symbols, including debug data from static runtime libraries.

Preserve `-static -static-libgcc -static-libstdc++`, C++ exceptions, RTTI, and executable security settings. Keep
`Debug` and `RelWithDebInfo` debugging information intact. Use integer-based conversion for error codes and countdowns;
avoid C++ stream and locale machinery. Route application diagnostics through the portable log and native read-only
control.

## Reference measurements

A Linux x64 cross-build with MinGW-w64 GCC 13-win32 produced these historical measurements. They are reference values,
not size limits; results vary with the compiler and runtime libraries.

| Build                                       |     Bytes |
| ------------------------------------------- | --------: |
| Previous Release                            | 2,506,211 |
| Previous Release with symbols stripped      | 1,019,392 |
| Size flags, LTO, unused-code removal, strip | 1,001,472 |
| Above plus stream-free output               |   178,688 |

The final executable was 92.9% smaller, imported only Windows-provided DLLs, and retained ASLR/NX.

## Validation

For changes that can affect binary size (application code, build/link settings, dependencies, or resources):

1. Save a baseline Release executable before the change, then rebuild with the same compiler, runtime libraries, target
   architecture, and configuration. Record before/after byte counts and percentage change. Investigate unexpected
   growth; fix regressions or explain necessary increases.
2. Inspect DLL imports. Only Windows-provided DLLs are acceptable: no `libgcc`, `libstdc++`, or `libwinpthread`
   dependency. OS-provided `msvcrt.dll` or UCRT API sets are expected. Static-link flags alone do not prove this.
3. Inspect the Windows GUI subsystem and the embedded icon, version, and Per-Monitor V2 manifest resources.
4. Verify ASLR/NX remain enabled. When changing build settings, also verify that `Debug` and `RelWithDebInfo` retain
   debugging information.

Measure and inspect a Linux cross-build with:

```sh
wc -c < build/linux-mingw-release/DoubleClickHotkey.exe
x86_64-w64-mingw32-objdump -p build/linux-mingw-release/DoubleClickHotkey.exe
```

On Windows, measure with PowerShell `(Get-Item build/windows-mingw-release/DoubleClickHotkey.exe).Length` and use the
toolchain's `objdump` with that path. `DYNAMIC_BASE` and `NX_COMPAT` identify ASLR and NX in the PE headers.

Manually verify affected Windows behavior: F13 handling, tray/duplicate activation (including cross-elevation), log
selection and scrolling, DPI/resizing, countdowns across sleep, and shutdown. Report unavailable checks as pending;
cross-compilation and binary inspection do not exercise runtime behavior.

## Tray conversion measurement

On Linux with MinGW-w64 GCC 13-win32, using the same `linux-mingw-release` preset before and after conversion:

| Build                   |           Bytes |
| ----------------------- | --------------: |
| Console baseline        |         178,688 |
| Native tray application |         241,152 |
| Increase                | 62,464 (34.96%) |

The increase provides the native window/tray, Unicode log presentation, bounded log buffer, and nonblocking countdown.
PE section inspection attributes about 36.6 KB to added code, 16.4 KB to resources, 7.7 KB to exception/unwind tables,
and 2.6 KB to imports; other section changes and alignment account for the remainder. The ICO itself is 13,304 bytes.
Static linking and stream-free formatting remain enabled.

The inspected executable uses the Windows GUI subsystem, imports only Windows-provided DLLs/API sets, embeds eight icon
sizes plus version/manifest resources, and retains ASLR/NX. Debug and RelWithDebInfo both retain `.debug_info`,
`.debug_abbrev`, and `.debug_line` sections. The build, 29 portable tests, and formatting checks pass. Native Windows
desktop checks remain pending on a Linux build host; cross-compilation does not verify tray interaction, foreground
activation, DPI, log selection/scrolling, or F13 delivery to another application.
