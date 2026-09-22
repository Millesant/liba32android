# Next Work

Repository integration remains on `bleeding` through PR #31 / runtime commit `2c3be26c05dff81be1f81c9565df5906c521a54c`. Active feature work is draft PR #32 / `005-elf32-dependency-loading` on the single branch `m4-elf32-dependency-loading`.

1. Gate T003 — bounded recursion, cycles, shared transitive objects, and aggregate rollback.
   - Status: IMPLEMENTED; validation pending.
   - Implementation commit: `b6c286f69516bcfd6f85c4a3e2b00e754c29bb38`.
   - Focused test head: `9bfbdd7e99a5daed6cfa6ee4b6c58c5c85cef2af`.
   - CI #191 was queued on the exact focused-test head; no final job result had been observed at checkpoint time.
   - Implemented behavior: Discovered/Loading/Loaded state; full direct-set acquisition before recursion; deterministic depth-first traversal; cycle/shared-object reuse by provider identity; global object/dependency-occurrence/image-byte accounting including duplicate acquisitions; max-depth enforcement; reverse successful-load-order rollback.
   - Focused coverage: A→B→C, A→B→A cycle, shared C, max-depth failure, later transitive ET_EXEC failure, recursive occurrence budget, recursive total-image budget, and preexisting mapping preservation.
   - Exact next action: reconcile CI #191 and the latest branch-head workflow after this checkpoint. If the latest head containing the implementation PASSes Linux A32 smoke, Android x86_64 probe, and Android arm64-v8a cross-build, mark T003 DONE and advance to T004. If not, inspect/fix only the observed regression.
   - DoD: authoritative containing-head CI has all three jobs PASS.

2. T004 — integrate the pinned real ARM32 fixture through the completed graph API.
   - Start only after T003 PASS.
   - Require automatic placement, `0x4000` alignment, zero dependency edges/provider calls, and neighboring real-fixture regressions green.
3. T005 — converge docs/state/spec and run final exact-head merge gate.

Hardware evidence remains BLOCKED until an accessible Android environment is available. Project license remains BLOCKED on maintainer choice.

Branch hygiene: keep the entire feature on `m4-elf32-dependency-loading`; do not create task-by-task or reconciliation branches.
