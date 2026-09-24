# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `Millesant/.gpt@609e6cb9cff9d00e241aa5437d9904fc7492f407` (v7.2.0).

## Active

`project-cleanup-v8` is IMPLEMENTED and awaiting exact-head validation.

Required closeout:

1. confirm the persisted tree contains the intended source/test/tool/documentation layout with no accidental path loss;
2. require Linux A32 smoke, Android x86_64 address-space probe, and Android arm64-v8a cross-build to pass on the exact implementation revision;
3. reconcile parent/child change records, evidence, `.agent/STATE.md`, and `.agent/NEXT.md` on a persistence-only closeout revision.

## After cleanup

No new runtime feature-scale implementation is selected. Choose the next runtime feature from accepted gaps rather than continuing cosmetic reorganization.

Independent evidence/decision follow-ups:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
