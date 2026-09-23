# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is DONE. Feature `007-elf32-relocations` is active.

1. T004 — pinned real ARM32 `R_ARM_GLOB_DAT` application.
   - Status: ACTIVE.
   - Dependency gate: T003 PASSed exact-head CI #214 / run `35890660951` at `41a93348c29fb884befba5ba8bad51ecf0d49665`; Linux passed 44/44 CTest including `elf32_relocation_apply`, Android x86_64 PASSed, and Android arm64-v8a PASSed.
   - Existing real-fixture oracle: main `.rel.dyn` has exactly two GLOB_DAT entries at linked offsets `0x82cc` / `0x82d0`, symbol indexes 2 / 3 for `fixture_bss` / `fixture_data`, and zero original words.
   - Implemented scope for the pending gate: dedicated `elf32_real_relocation_apply` CTest graph-loads the pinned fixture, resolves `fixture_bss` / `fixture_data` through feature 006, snapshots all readable loaded segments plus mapping permissions, applies object-0 relocations, requires the two target words to equal the resolved guest values, and permits no other segment-byte or mapping/permission changes; provider calls remain zero and initialized data/BSS are revalidated.
   - Exact next action: run exact-head CI and require `elf32_real_relocation_apply`, neighboring real fixture/linker/symbol tests, `elf32_relocation_apply`, Android x86_64, and Android arm64-v8a to PASS.
   - Stop condition: real-fixture relocation application is exact-head validated and persisted. Do not start final T005 convergence until T004 is green.

2. T005 remains blocked on T004.
   - Final architecture/README/spec/state convergence and exact-head feature gate are not admitted until the real-fixture mutation oracle passes.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items into feature 007 unless a relocation acceptance criterion makes one REQUIRED_NOW.
