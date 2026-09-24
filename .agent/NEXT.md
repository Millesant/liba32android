# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `Millesant/.gpt@1df73390ad04c0909e3ca42daa690789f150f1ca` (v7.1.0).

Feature `010-elf32-gnu-relro` and maintenance change `repository-organization-v7` are DONE. Their persistence-only closeout head `38304a2e4fcb3bdb96cd77785256afe135c8653a` passed GitHub Actions CI #236 / run `35979083056` in all three required lanes.

Active maintenance change: `test-infrastructure-cleanup-v7`.

1. T001 — shared real-fixture binary/guest-word helpers — ACTIVE.
2. T002 — collapse repeated CMake test executable setup — PLANNED.
3. T003 — persisted-tree inspection, exact-head CI, and convergence — PLANNED.

This cleanup is behavior-preserving: runtime source/contracts, CTest names, fixture arguments, and Android artifact paths must not change.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
