# Next Work

Repository state is on `bleeding` at `9bb52b1e50bf818f6326424975582372db1353cd`; PR #32 / `005-elf32-dependency-loading` remains DONE. Feature `006-elf32-symbol-resolution` is now readiness-checked.

1. T001 — add hash metadata and bounded dynamic-symbol indexing.
   - Status: READY.
   - Spec: `specs/006-elf32-symbol-resolution/{requirements,design,tasks}.md`.
   - Scope: recognize/rebase `DT_HASH` and `DT_GNU_HASH`, add bounded SysV/GNU index parsing, derive a trustworthy dynamic-symbol count without section headers, and validate the complete implied `Elf32_Sym` range.
   - Exact next action: implement `elf32_symbol_lookup.{h,cpp}`, extend linker metadata hash descriptors/tests, wire the focused host test into CMake/CTest, then run exact-head CI.
   - DoD: T001 focused tests PASS, neighboring linker tests remain green, Android cross-build still compiles the new source, and durable task/state records the exact evidence.

2. T002 — exact-name per-object symbol lookup.
   - Status: BLOCKED on T001.

3. T003 — deterministic graph-local BFS symbol lookup.
   - Status: BLOCKED on T002.

4. T004 — pinned real ARM32 GNU-hash fixture integration.
   - Status: BLOCKED on T003.

5. T005 — documentation/state convergence and final exact-head gate.
   - Status: BLOCKED on T004.

Android native tombstone/backtrace evidence remains BLOCKED on an accessible device environment. Project license remains BLOCKED on maintainer choice.
