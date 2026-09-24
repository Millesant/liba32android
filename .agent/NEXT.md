# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `Millesant/.gpt@1df73390ad04c0909e3ca42daa690789f150f1ca` (v7.1.0).

Feature `010-elf32-gnu-relro` remains the active runtime change. Repository workflow/build cleanup is tracked separately as `repository-organization-v7`.

## 010 — ELF32 GNU RELRO

1. T001 — validate/expose `PT_GNU_RELRO` metadata — VERIFIED at CI #231 / run `35942933233` on `e3ea30a9445c86546933d008ee5a336bd9e91e8e`.
2. T002 — bounded transactional RELRO sealing — VERIFIED at CI #232 / run `35943552213` on `655f933f46c4bb28e5c36fe34b628b92af1f679b`.
3. T003 — real post-relocation GNU RELRO sealing — IMPLEMENTED; exact-head validation NOT RUN.
   - The pinned ARMv7 fixture is loaded through the dependency graph, supported relocations are applied, both real GLOB_DAT targets are checked inside GNU RELRO, sealing preserves relocated bytes, post-seal writes fail, and non-RELRO page permissions are checked unchanged.
   - Next gate: validate this integration on the resulting exact cleanup revision; do not treat T001/T002 historical green as T003 validation.
4. T004 — docs/spec/change/state convergence and final exact-head gate — QUEUED, depends on T003 PASS.

Explicit non-goals remain lazy binding/`DT_PLTGOT`, Android RELRO sharing/serialization, version-aware/process-wide interposition, protected requester semantics, TLS, IFUNC, broader relocation forms, permission broadening, and guest execution.

## repository-organization-v7

1. T001 — v7 project/current-spec migration — ACTIVE.
2. T002 — modularize root CMake declarations without target/test/artifact behavior changes — PLANNED.
3. T003 — centralize duplicated private ELF32 decode/address helpers — PLANNED.
4. T004 — exact-head CI + state/evidence convergence — PLANNED.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
