# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution`, `007-elf32-relocations`, and `008-elf32-plt-relocation-metadata` are DONE. Feature `009-elf32-jump-slot-relocations` is selected as the next bounded M4 slice under control revision `9240ab19507b86491398a5c9fdf0deb58e2fdc91`.

1. T001 — PLT REL planning and reference resolution — VERIFIED at CI #226 / run `35938429972` on `fe12b6de747884a18d1214f564559d94937d8974`.
   - Base: `bbad7db39e430ba87a373841e3ede954696c85c5`.
   - Scope: add `R_ARM_JUMP_SLOT`, reuse the proven relocation planner/resolver internally, expose separate read-only PLT plan/resolution APIs over `plt_rel_table`, accept only JUMP_SLOT, and preserve main-`DT_REL` behavior exactly.
   - Validation: focused synthetic PLT plan/reference coverage plus existing relocation/dependency/symbol suites and exact-head Linux + Android CI.
   - Linux A32 smoke, Android x86_64, and Android arm64-v8a all PASSed.

2. T002 — transactional eager JUMP_SLOT application — IMPLEMENTED; validation NOT RUN.
   - Prepared semantics: JUMP_SLOT writes `S`, ignores the in-place word as an addend, unresolved WEAK writes zero, strong misses are pre-write failures, and late write failures reuse verified reverse rollback.
   - Exact next action: commit T002 source/tests and verify exact-head Linux + Android CI before starting the real fixture.
3. T003 — reproducible provider/consumer ARMv7 fixture + graph-backed real application — QUEUED, depends on T002 PASS.
4. T004 — docs/spec/change/state convergence and final exact-head gate — QUEUED, depends on T003 PASS.

Explicit non-goals: lazy binding/`DT_PLTGOT`, version-aware or process-wide interposition policy, TLS, IFUNC, RELRO, broader relocation forms, permission broadening, and guest execution.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
