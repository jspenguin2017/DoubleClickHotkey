# Build, tooling, and test infrastructure code review

## Scope and basis

- Repository-wide review segment: `CMakeLists.txt`, `CMakePresets.json`, `cmake/`,
  `tests/CMakeLists.txt`, test dependency/include wiring, `package.json`, `package-lock.json`,
  `scripts/`, asset generation and resource inputs, repository/editor configuration, README,
  and `docs/`.
- Basis: commit `c502829e0f39009868ec79818186779d42e6bb6b`; initially clean worktree.
- Reviewed supported host/toolchain selection, target dependencies, C++17 requirements,
  native test execution and cross-build discovery, dependency pinning, static release linking,
  optimization/debug flags, resource dependencies, npm command dispatch, and documented workflows.
- Formatting itself, individual test cases/assertions/fixture data, third-party source, and
  generated implementation code were excluded. Existing build products were inspected only
  as validation evidence.

## Findings

No verified findings in this segment. This does not establish that the code is defect-free.

## Unresolved questions

None beyond the platform and first-install checks listed below.

## Checks and results

### Native build and test infrastructure

- Configured a separate Debug build at `/tmp/double-click-hotkey-review-native`, with the
  app disabled and GoogleTest resolved from the existing source at
  `build/linux-native-debug/_deps/googletest-src`. `FETCHCONTENT_FULLY_DISCONNECTED=ON`
  prevented dependency fetching. Configuration and compilation succeeded with GCC 13.3.0.
- The freshly built `/tmp/double-click-hotkey-review-native/tests/double_click_hotkey_tests`
  executable passed all 40 tests.
- `ctest --preset linux-native-debug --test-dir /tmp/double-click-hotkey-review-native`
  passed all 40 tests, but CTest selected the preset's repository build directory. This was
  a check of the existing native preset, not the fresh temporary executable; the latter was
  run directly as described above.
- Confirmed that the test target links the portable core and GoogleTest, the entrypoint is
  compiled under its test-only name, test includes are portable, and GoogleTest discovery uses
  `DISCOVERY_MODE PRE_TEST`. Only native-host CTest was invoked.

### Release artifact and resources

- `ninja -C build/linux-mingw-release -n` reported no work pending for the existing Release build.
- Existing `build/linux-mingw-release/DoubleClickHotkey.exe`: **232,960 bytes**. No application
  changes were made, so a before/after size regression comparison is not applicable.
- MinGW `objdump -p` identified an x64 Windows GUI executable with `HIGH_ENTROPY_VA`,
  `DYNAMIC_BASE`, and `NX_COMPAT` enabled.
- Imports are limited to Windows DLLs: ADVAPI32, COMCTL32, GDI32, KERNEL32, SHELL32, USER32,
  msvcrt, and Windows `api-ms-win-core-*` API sets. No libgcc, libstdc++, or libwinpthread
  DLL import was present.
- Resource inspection confirmed icon, group icon, version, and manifest entries. The manifest
  XML declares as-invoker execution and Per-Monitor V2 DPI awareness.
- The existing Debug executable retains DWARF debug sections; Release has no debug sections.
- The checked-in ICO contains one PNG-compressed 256×256 entry, consistent with the converter
  configuration and documentation. The source SVG and native manifest parse as XML.

### Tooling and configuration

- Both JavaScript scripts pass `node --check`.
- Manifest and lockfile root dependency/engine requirements agree; installed direct development
  dependencies match the pinned versions. Every registry package entry has an integrity hash.
- Checked npm task routing against host-specific configure/build/test presets and documentation.
- Checked resource dependency declarations and all C++ file registrations; no C++ source/header
  is missing from `DOUBLE_CLICK_HOTKEY_FORMAT_FILES`, and all listed files exist.
- Final tracked-file diff is empty. Review reports are the only source-tree additions.

## Limitations

- No dependencies were installed, updated, or repaired. Clean first-time downloads, npm
  installation on a fresh machine, and network/registry failure behavior were not exercised.
- Native Windows configuration and testing were not available. Cross-build artifact inspection
  does not establish Windows runtime behavior.
- The existing Release and Debug artifacts were inspected; Release was verified current by
  the build graph. No new RelWithDebInfo build was produced. Its debug retention was reviewed
  through the configuration-specific compile/link settings.
- The icon conversion command was not executed because it overwrites the reviewed asset.
  Scaled icon appearance requires a Windows desktop check.
- No formatting suite was run: formatting is explicitly outside the review scope.
