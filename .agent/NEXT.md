# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution` and `007-elf32-relocations` are DONE. Feature `008-elf32-plt-relocation-metadata` T001 is VERIFIED at `9a81ed71a027beb166970bcf137bac9a71112f98`.

1. T002 — documentation/spec/state convergence and final feature gate — ACTIVE.
   - T001 evidence: CI #222 / run `35933694619`; Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build all completed successfully.
   - Accepted boundary: validated guest-only `DT_JMPREL` / `DT_PLTRELSZ` / `DT_PLTREL=DT_REL` metadata only. The pinned real fixture explicitly reports no PLT REL metadata.
   - Preserve non-goals: no PLT relocation entry decode, no `R_ARM_JUMP_SLOT`, no lazy binding/`DT_PLTGOT`, and no guest-memory writes.
   - Exact next action: commit T002 docs/spec/state convergence on `bleeding`, run the final exact-head Linux + Android CI gate, and on PASS persist feature 008 as DONE.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items or later `R_ARM_JUMP_SLOT` application into feature 008.
