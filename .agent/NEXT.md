# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `Millesant/.gpt@1df73390ad04c0909e3ca42daa690789f150f1ca` (v7.1.0).

Feature `010-elf32-gnu-relro`, `repository-organization-v7`, and `test-infrastructure-cleanup-v7` are DONE. The test-infrastructure persistence-only closeout head `0b51069def55cfe3ce35621845d948ea2401a38f` passed CI #239 / run `35980275074`.

Active maintenance change: `elf32-header-layering-v7`.

1. T001 — extract the shared `Elf32LoadError` contract and preserve loader API exposure — ACTIVE.
2. T002 — remove unnecessary loader-header dependencies from load plan/placement and pass exact-head Linux + Android CI — PLANNED.

This cleanup must not change ELF load behavior, enum ordering, public function signatures, or compatibility claims.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
