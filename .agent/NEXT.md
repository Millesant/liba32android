# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution` and `007-elf32-relocations` are DONE. Feature `008-elf32-plt-relocation-metadata` T001 implementation is prepared against spec head `deaa2f09d00de578953a381e4b3b0389868767ff`; validation is NOT RUN.

1. T001 — validate and expose PLT REL metadata — ACTIVE.
   - Prepared scope: `DT_JMPREL` + `DT_PLTRELSZ` + `DT_PLTREL=DT_REL` collection, checked rebasing/range/readability, additive guest-only `plt_rel_table`, explicit PLT metadata errors, and focused synthetic coverage.
   - Preserve feature 007: no PLT entry decoding, no `R_ARM_JUMP_SLOT`, no lazy binding, and no guest-memory writes.
   - Exact next action: commit T001 on `bleeding`, then verify exact-head Linux CTest + Android x86_64 + Android arm64-v8a CI. On PASS, start T002 convergence.

2. T002 — docs/state/spec convergence and final feature gate — QUEUED, depends on T001 PASS.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items or later `R_ARM_JUMP_SLOT` application into feature 008.
