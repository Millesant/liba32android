# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution`, `007-elf32-relocations`, and `008-elf32-plt-relocation-metadata` are DONE. Feature `009-elf32-jump-slot-relocations` T001-T003 are VERIFIED under control revision `9240ab19507b86491398a5c9fdf0deb58e2fdc91`.

1. T004 — docs/spec/change/state convergence and final exact-head gate — ACTIVE.
   - T001: CI #226 / run `35938429972` PASS at `fe12b6de747884a18d1214f564559d94937d8974`.
   - T002: CI #227 / run `35938885569` PASS at `666a15ab2edaebdfa3c0f6817dca30e2e2e7a931`.
   - T003: CI #228 / run `35939575947` PASS at `815386149732201ce5b64e1b5ad207079491eb80`, including reproducible provider/consumer DSOs, observed `DT_NEEDED` + `R_ARM_JUMP_SLOT fixture_import`, and graph-backed real eager application.
   - Accepted boundary: separate main-REL and PLT-REL APIs; PLT accepts only eager JUMP_SLOT = `S`; no lazy binding/`DT_PLTGOT`; no combined main+PLT atomic transaction; no guest execution.
   - Exact next action: commit the prepared convergence head, verify final exact-head Linux A32 smoke + Android x86_64 + Android arm64-v8a CI, then mark feature 009 DONE.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items or lazy binding into feature 009 closeout.
