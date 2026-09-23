# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution` and `007-elf32-relocations` are DONE. Feature `008-elf32-plt-relocation-metadata` T001 implementation passed CI #221 at `47070bef73acd14464137789585abd4972878ab4`, but convergence found one REQUIRED_NOW acceptance gap: AC8 did not explicitly assert the pinned real fixture has no PLT REL metadata.

1. T001 — validate and expose PLT REL metadata — ACTIVE.
   - Implementation CI: #221 / run `35933289817` PASSed all three jobs at `47070bef73acd14464137789585abd4972878ab4`.
   - Prepared gap fix: real-fixture linker-metadata integration now requires no `DT_JMPREL`, `DT_PLTRELSZ`, or `DT_PLTREL` tags and no validated `plt_rel_table`.
   - Preserve feature 007: no PLT entry decoding, no `R_ARM_JUMP_SLOT`, no lazy binding, and no guest-memory writes.
   - Exact next action: commit the AC8 oracle and re-run exact-head CI. On PASS, mark T001 PASS and start T002 convergence.

2. T002 — docs/state/spec convergence and final feature gate — QUEUED, depends on T001 PASS.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items or later `R_ARM_JUMP_SLOT` application into feature 008.
