# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution`, `007-elf32-relocations`, `008-elf32-plt-relocation-metadata`, and `009-elf32-jump-slot-relocations` are DONE. Feature `010-elf32-gnu-relro` is selected as the next bounded M4 slice under control revision `9240ab19507b86491398a5c9fdf0deb58e2fdc91`.

1. T001 — validate/expose `PT_GNU_RELRO` metadata — IMPLEMENTED; exact-head validation NOT RUN.
   - Base: `fd6670f3d40d51ad6fa5995b27ede87e4aa1b51c`.
   - Scope: recognize zero-or-more GNU RELRO program headers in the shared load plan, validate non-empty/overflow/page-rounded readable PT_LOAD coverage, expose load-biased exact + mapped ranges, and keep original PT_LOAD permissions unchanged.
   - Real oracle: the existing pinned ARMv7 fixture must retain one GNU RELRO program header and remain writable before the future explicit sealing step.
   - Exact next action: commit T001 source/tests/specs and verify exact-head Linux + Android CI.
2. T002 — bounded transactional RELRO sealing — QUEUED, depends on T001 PASS.
3. T003 — real post-relocation GNU RELRO sealing — QUEUED, depends on T002 PASS.
4. T004 — docs/spec/change/state convergence and final exact-head gate — QUEUED, depends on T003 PASS.

Explicit non-goals: lazy binding/`DT_PLTGOT`, Android RELRO sharing/serialization, version-aware/process-wide interposition, protected requester semantics, TLS, IFUNC, broader relocation forms, permission broadening, and guest execution.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
