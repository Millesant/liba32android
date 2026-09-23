# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution` and `007-elf32-relocations` are DONE. Feature `008-elf32-plt-relocation-metadata` is selected and readiness-checked at base `ef5f08eb6d184c12b252ac6f59994e8efed21d5b`.

1. T001 — validate and expose PLT REL metadata — READY.
   - Scope: `DT_JMPREL` + `DT_PLTRELSZ` + `DT_PLTREL=DT_REL` collection, checked rebasing/range/readability, additive guest-only `plt_rel_table`, and focused synthetic tests.
   - Preserve feature 007: no PLT entry decoding, no `R_ARM_JUMP_SLOT`, no lazy binding, and no guest-memory writes.
   - Exact next action: implement T001 in `src/elf/elf32_linker_metadata.{h,cpp}` and `tests/elf32_linker_metadata.cpp`, then run exact-head CI.

2. T002 — docs/state/spec convergence and final feature gate — QUEUED, depends on T001 PASS.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items or later `R_ARM_JUMP_SLOT` application into feature 008.
