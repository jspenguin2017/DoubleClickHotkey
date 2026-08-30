# Code Review: Build, Configuration, and Test Infrastructure

## Reviewed scope and basis

- Scope: `CMakeLists.txt`, `CMakePresets.json`, `cmake/`, `tests/CMakeLists.txt`, `scripts/`, `package.json`,
  `package-lock.json`, repository setup/editor/ignore configuration, `README.md`, and `AGENTS.md`.
- Review basis: repository state at commit `9e03888e53b069b9f0dd2ffdeef8ba97c9774daf` on 2026-08-30.
- Review method: traced host-to-preset selection, CMake target and usage-requirement propagation, MinGW-w64 toolchain
  selection, release linkage, GoogleTest acquisition and discovery, production-main test linkage, npm dependency locking,
  documented prerequisites, formatting entry points, install output, and developer commands.

## Findings

### B-1: The Node.js support contract contradicts the package manifest

- Severity: **Low**
- References: `package.json:6-8`, `package-lock.json:14-16`, `README.md:77-78`, and `AGENTS.md:32`.
- Problem: The package and lockfile declare `node >=24.19.0`, while the user documentation says Node.js 20 or newer is
  sufficient and recommends 22.12 or newer; the repository instructions likewise recommend 22.12 or newer. These ranges
  disagree about whether Node.js 20 and 22 are supported.
- Impact: Developers following the documented prerequisites receive an unsupported-engine warning, and installation can
  fail outright in environments that enforce engine ranges. CI or package-manager policy can therefore reject a Node
  version the repository explicitly tells users to install.
- Recommendation: Choose one actual minimum and apply it consistently to `package.json`, `package-lock.json`,
  `README.md`, and `AGENTS.md`. If Node 20/22 is intentionally supported, lower the manifest range after validating the
  scripts; if 24.19 is required, update both documents and state the reason for the narrow minimum.

### B-2: The documented cache-bypassing format command does not exist

- Severity: **Low**
- References: `AGENTS.md:34-36` and `package.json:16-20`.
- Problem: `AGENTS.md` directs developers to run `npm run format:nocache`, but `package.json` defines no
  `format:nocache` script. Running the documented command exits with npm's `Missing script: "format:nocache"` error.
  The existing `format` command also does not enable Prettier caching, so the documentation's cache distinction has no
  implementation in the current scripts.
- Impact: The prescribed full formatting workflow is broken and can mislead maintainers into thinking a cached pass was
  bypassed when no such workflow exists.
- Recommendation: Either add a correctly implemented `format:nocache` script and define how it differs from `format`,
  or remove the stale instruction and direct maintainers to the existing uncached command.

## Unresolved questions

None.

## Checks and areas not covered

- `npm ls --depth=0` confirmed the already-installed, locked Prettier 3.9.6 dependency.
- `npm run test` configured and built `linux-native-debug`, then passed all 27 discovered tests.
- `npm run build` configured and built both `linux-mingw-debug` and `linux-mingw-release` successfully with MinGW-w64
  GCC 13.
- `npm run format:nocache` reproduced the missing-script failure. npm could not write its optional user-level error log
  under the review sandbox, which did not affect the observed command result.
- The Windows-hosted `windows-mingw-*` presets were not exercised from the Linux review host.
- Dependency bootstrap from an empty cache was not exercised; the review used the repository's existing dependencies and
  did not install, update, or repair anything.
- Individual test cases, fixtures, assertions, and coverage adequacy were not reviewed. Test source participation and
  linkage were considered only as test infrastructure.
- Formatting style and formatter-owned whitespace were excluded from review. The legal sufficiency of `LICENSE` was not
  assessed.
