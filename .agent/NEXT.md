# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is active.

1. T002 — exact-name per-object symbol lookup.
   - Status: ACTIVE.
   - Dependency gate: T001 PASSed exact-head CI #203 / run `35758444356` at `45cd3e5a322263f53dc02277a9d0e801849515db`; Linux passed 40/40 CTest including `elf32_symbol_index`, Android x86_64 PASSed, and Android arm64-v8a PASSed.
   - Implemented scope for the pending gate: SysV/GNU hash-directed exact-name lookup; byte-explicit `Elf32_Sym` decoding; bounded STRTAB candidate reads; GLOBAL/WEAK + DEFAULT/PROTECTED eligibility; LOCAL/UNDEF/HIDDEN/INTERNAL skip; first-definition weak behavior; ABS vs checked rebased guest values; explicit unsupported binding/type/visibility/section/versioning failures; bounded malformed-chain handling; read-only tests.
   - Exact next action: run exact-head CI on the T002 implementation commit and require `elf32_symbol_index`, all neighboring ELF/linker tests, Android x86_64, and Android arm64-v8a to PASS.
   - On PASS: mark T002 DONE, make T003 READY, and implement deterministic graph-local BFS lookup.

2. T003 — deterministic graph-local BFS symbol lookup.
   - Status: BLOCKED on T002 exact-head validation.

3. T004 — pinned real ARM32 GNU-hash fixture integration.
   - Status: BLOCKED on T003.

4. T005 — documentation/state convergence and final exact-head gate.
   - Status: BLOCKED on T004.

Android native tombstone/backtrace evidence remains BLOCKED on an accessible device environment. Project license remains BLOCKED on maintainer choice.
