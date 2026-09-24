# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution`, `007-elf32-relocations`, `008-elf32-plt-relocation-metadata`, and `009-elf32-jump-slot-relocations` are DONE. Feature `010-elf32-gnu-relro` is active under control revision `9240ab19507b86491398a5c9fdf0deb58e2fdc91`.

1. T001 — validate/expose `PT_GNU_RELRO` metadata — VERIFIED at CI #231 / run `35942933233` on `e3ea30a9445c86546933d008ee5a336bd9e91e8e`.
2. T002 — bounded transactional RELRO sealing — VERIFIED at CI #232 / run `35943552213` on `655f933f46c4bb28e5c36fe34b628b92af1f679b`.
   - Linux A32 smoke PASSed 48/48 CTest including `elf32_relro_seal`; Android x86_64 and Android arm64-v8a also PASSed.
   - Validated semantics: complete preflight before mutation, declared-page bound, overlap deduplication, idempotent already-R pages, RW -> R sealing only, and reverse rollback machinery for later protection failure.
3. T003 — real post-relocation GNU RELRO sealing — IMPLEMENTED; exact-head validation NOT RUN.
   - Prepared integration loads the pinned ARMv7 fixture through the dependency graph, applies supported main relocations plus the empty eager-PLT path, confirms both real GLOB_DAT targets lie inside GNU RELRO, seals the range, preserves relocated bytes, rejects post-seal writes, and checks non-RELRO page permissions remain unchanged.
   - Exact next action: commit T003 source/tests/CI/state and verify exact-head Linux + Android CI.
4. T004 — docs/spec/change/state convergence and final exact-head gate — QUEUED, depends on T003 PASS.

Explicit non-goals: lazy binding/`DT_PLTGOT`, Android RELRO sharing/serialization, version-aware/process-wide interposition, protected requester semantics, TLS, IFUNC, broader relocation forms, permission broadening, and guest execution.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
