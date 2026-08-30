# Code Review: Build, Setup, and Repository Metadata

## Reviewed scope and basis

- Scope: `CMakeLists.txt`, `CMakePresets.json`, `cmake/`, `tests/CMakeLists.txt`, `scripts/cmake.mjs`, `README.md`,
  `package.json`, `package-lock.json`, `.clang-format`, `.prettierignore`, `.prettierrc.json`, `.gitignore`, and
  `LICENSE`.
- Revision: `985365d` (`main`), with a clean worktree confirmed before the review update began.
- Review method: revalidating the previous build/setup review against the CMake target graph, preset matrix, compiler
  and linker settings, source-file membership, dependency and test configuration, formatting tools, setup guidance, and
  repository exclusions introduced by the restructure.

## Findings

No build/setup finding from the previous review remains applicable. The legacy Visual Studio solution and project files
were removed, so their conflicting Visual Studio authoring and platform-toolset versions no longer exist. The supported
build paths are now defined by CMake and the MinGW-w64 presets documented in `README.md`.

## Unresolved questions

- Are Releases-page binaries produced, signed, and verified through a separate process? No CI, release workflow, or
  artifact-provenance configuration is present in this repository, and external release artifacts were outside this
  review's local scope.

## Checks and areas not covered

- `cmake --preset linux-native-debug`, `cmake --build --preset linux-native-debug`, and
  `ctest --preset linux-native-debug` passed; all 10 discovered tests passed.
- `cmake --preset linux-mingw-debug` and `cmake --build --preset linux-mingw-debug` passed, as did the corresponding
  `linux-mingw-release` commands.
- The CMake target source lists agree with the restructured tree. GoogleTest is pinned to an upstream revision and
  archive checksum, and the application-controller and hotkey-policy tests are tracked under `tests/`.
- The release executable is a 64-bit Windows console binary. Its import table contains only `KERNEL32.dll`,
  `msvcrt.dll`, and `USER32.dll`; it does not depend on companion MinGW-w64 runtime DLLs.
- The Windows executable and cross-compiled tests were not run because no Windows desktop or separately configured
  compatibility environment is available in this Linux workspace.
- No application source, build configuration, or test file was modified as part of this review update.
