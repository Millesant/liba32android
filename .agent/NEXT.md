# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution`, `007-elf32-relocations`, `008-elf32-plt-relocation-metadata`, and `009-elf32-jump-slot-relocations` are DONE. Feature `010-elf32-gnu-relro` is active under control revision `9240ab19507b86491398a5c9fdf0deb58e2fdc91`.

1. T001 — validate/expose `PT_GNU_RELRO` metadata — VERIFIED at CI #231 / run `35942933233` on `e3ea30a9445c86546933d008ee5a336bd9e91e8e`.
   - Linux A32 smoke PASSed 47/47 CTest including `elf32_relro_metadata`; Android x86_64 and Android arm64-v8a also PASSed.
   - The pinned ARMv7 fixture reports one GNU RELRO range at linked VA `0x826c`, memsz `0xd94`, page-rounded mapping size `0x1000`, and remains writable immediately after load.
2. T002 — bounded transactional RELRO sealing — IMPLEMENTED; exact-head validation NOT RUN.
   - Prepared semantics: caller-selected declared-page bound, complete metadata/mapping/permission preflight, overlap deduplication, idempotent already-read-only handling, RW -> R sealing only, and reverse permission rollback on a later protect failure.
   - Exact next action: commit T002 source/tests/state and verify exact-head Linux + Android CI before T003.
3. T003 — real post-relocation GNU RELRO sealing — QUEUED, depends on T002 PASS.
4. T004 — docs/spec/change/state convergence and final exact-head gate — QUEUED, depends on T003 PASS.

Explicit non-goals: lazy binding/`DT_PLTGOT`, Android RELRO sharing/serialization, version-aware/process-wide interposition, protected requester semantics, TLS, IFUNC, broader relocation forms, permission broadening, and guest execution.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
