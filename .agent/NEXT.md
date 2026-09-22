# Next Work

Repository integration remains on `bleeding` through PR #31 / runtime commit `2c3be26c05dff81be1f81c9565df5906c521a54c`. Active feature work is existing draft PR #32 / `005-elf32-dependency-loading` on `m4-elf32-dependency-loading`.

1. Re-gate T001-T003 with real build/test wiring.
   - Status: ACTIVE.
   - Finding: CI #193 PASSed all three jobs, but Linux CTest ran only 37 pre-existing tests; `elf32_dependency_loader.cpp` and `tests/elf32_dependency_loader.cpp` were not wired into CMake/CTest.
   - Current fix: add the loader source to `liba32android`, add `elf32_dependency_loader_test`, and register `elf32_dependency_loading`.
   - Exact next action: run exact-head CI and confirm Linux CTest includes the dependency-loader test while Android arm64-v8a also compiles the loader source.
   - DoD: all three CI jobs PASS and the Linux test log explicitly contains `elf32_dependency_loading`.

2. T004 — integrate the pinned real ARM32 fixture through the dependency graph API.
   - Start only after the corrected T001-T003 gate PASSes.
   - Require automatic placement/load, `0x4000` alignment, zero dependency edges/provider calls, and neighboring real-fixture regressions green.

3. T005 — converge architecture/docs/state/spec and run the final exact-head feature gate.

Hardware evidence remains BLOCKED until an accessible Android environment is available. Project license remains BLOCKED on maintainer choice.
