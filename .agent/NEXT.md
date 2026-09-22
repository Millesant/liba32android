# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is active.

1. T003 — deterministic graph-local breadth-first symbol lookup.
   - Status: ACTIVE.
   - Dependency gate: T002 PASSed exact-head CI #204 / run `35759553586` at `5f21c8ed48f458f7f3d909fff39523d9ebf9b7e0`; Linux passed 40/40 CTest, Android x86_64 PASSed, and Android arm64-v8a PASSed.
   - Implemented scope for the pending gate: BFS from a selected graph object using stored dependency-edge order; object-vector order is ignored as scope; repeated/cyclic/shared targets are visited once; first eligible definition wins; no-SYMTAB objects are skipped; earlier malformed/versioned searchable objects fail; scope-object ceiling and invalid graph edges are explicit.
   - Exact next action: run exact-head CI on the T003 implementation commit and require `elf32_symbol_index`, neighboring linker/dependency tests, Android x86_64, and Android arm64-v8a to PASS.
   - On PASS: mark T003 DONE, make T004 READY, and integrate the pinned real ARM32 GNU-hash fixture.

2. T004 — pinned real ARM32 GNU-hash fixture integration.
   - Status: BLOCKED on T003 exact-head validation.

3. T005 — documentation/state convergence and final exact-head gate.
   - Status: BLOCKED on T004.

Android native tombstone/backtrace evidence remains BLOCKED on an accessible device environment. Project license remains BLOCKED on maintainer choice.
