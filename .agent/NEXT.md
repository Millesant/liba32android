# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is DONE. Feature `007-elf32-relocations` is active.

1. T003 — transactional `R_ARM_NONE` / `R_ARM_RELATIVE` / `R_ARM_GLOB_DAT` / `R_ARM_ABS32` application.
   - Status: ACTIVE.
   - Dependency gate: T002 PASSed exact-head CI #212 / run `35889244367` at `650d7b262540360ba2395a802ba7d7766566d544`; Linux passed 43/43 CTest, Android x86_64 PASSed, and Android arm64-v8a PASSed.
   - Existing contracts: T001 builds a complete read-only bounded REL plan with original-word snapshots; T002 resolves symbol-bearing references read-only, including unresolved WEAK -> `S=0`.
   - Implemented scope for the pending gate: complete final-word calculation before mutation; RELATIVE symbol-index-zero enforcement; `RELATIVE = B + A`, `GLOB_DAT = S`, `ABS32 = S + A` modulo 2^32; NONE no-write behavior; table-order writes; failed-target state check; reverse restoration of earlier writes with readback verification; explicit rollback-failure reporting; no permission changes.
   - Validation added: dedicated `elf32_relocation_apply` CTest covering formulas, nonzero-addend GLOB_DAT suppression, pre-write failures, successful late-write rollback, and injected rollback failure.
   - Exact next action: run exact-head CI and require the new apply CTest, existing plan/real-plan tests, all neighboring ELF/linker/symbol tests, Android x86_64, and Android arm64-v8a to PASS.
   - Stop condition: T003 synthetic mutation/rollback behavior is validated and persisted. Do not begin real-fixture relocation application until the T003 gate is green.

2. T004-T005 remain dependency-ordered in `specs/007-elf32-relocations/tasks.md`.
   - Pinned real-fixture mutation and final feature convergence are not admitted into T003 until its exact-head gate passes.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items into feature 007 unless a relocation acceptance criterion makes one REQUIRED_NOW.
