# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is active.

1. T004 — pinned real ARM32 GNU-hash fixture integration.
   - Status: ACTIVE.
   - Dependency gate: T003 PASSed exact-head CI #205 / run `35760283793` at `f3997d037f7f5a29b1666dd9a6f5a566b249a2cc`; Linux passed 40/40 CTest, Android x86_64 PASSed, and Android arm64-v8a PASSed.
   - Implemented scope for the pending gate: load the pinned fixture through the dependency graph; require a GNU-hash symbol index; resolve `fixture_add`, `fixture_data`, and `fixture_bss` from object 0; read back initialized data/BSS through resolved guest values; require the function value to land in an executable loaded segment; compare loaded-segment bytes/mappings/permissions before and after lookup.
   - Exact next action: run exact-head CI on the T004 implementation commit and require `elf32_real_symbol_lookup`, neighboring real fixture/linker/dependency tests, Android x86_64, and Android arm64-v8a to PASS.
   - On PASS: mark T004 DONE, make T005 READY, and converge architecture/README/spec/state documentation before the final feature gate.

2. T005 — documentation/state convergence and final exact-head gate.
   - Status: BLOCKED on T004 exact-head validation.

Android native tombstone/backtrace evidence remains BLOCKED on an accessible device environment. Project license remains BLOCKED on maintainer choice.
