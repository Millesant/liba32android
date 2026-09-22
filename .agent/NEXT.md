# Next Work

Repository integration is on `bleeding`; PR #32 / `005-elf32-dependency-loading` remains DONE. Feature `006-elf32-symbol-resolution` is active.

1. T001 — hash metadata and bounded dynamic-symbol indexing.
   - Status: ACTIVE.
   - Implemented scope for the pending gate: `DT_HASH` / `DT_GNU_HASH` metadata descriptors; bounded SysV/GNU index construction; hash-count/index/termination checks; complete inferred `Elf32_Sym` range validation; focused metadata/index tests; CMake/CTest wiring.
   - Contract: `specs/006-elf32-symbol-resolution/{requirements,design,tasks}.md`; public resource/read-only guarantees are documented in `src/elf/elf32_symbol_lookup.h`.
   - Exact next action: run CI at the exact implementation commit and require Linux `elf32_symbol_index` plus all neighboring tests, Android x86_64 probe cross-build, and Android arm64-v8a cross-build to PASS.
   - On PASS: mark T001 DONE, make T002 READY, and implement exact-name per-object lookup.

2. T002 — exact-name per-object symbol lookup.
   - Status: BLOCKED on T001 exact-head validation.

3. T003 — deterministic graph-local BFS symbol lookup.
   - Status: BLOCKED on T002.

4. T004 — pinned real ARM32 GNU-hash fixture integration.
   - Status: BLOCKED on T003.

5. T005 — documentation/state convergence and final exact-head gate.
   - Status: BLOCKED on T004.

Android native tombstone/backtrace evidence remains BLOCKED on an accessible device environment. Project license remains BLOCKED on maintainer choice.
