# Next Work

Repository integration remains on `bleeding` through PR #31 / runtime commit `2c3be26c05dff81be1f81c9565df5906c521a54c`. Active feature work is draft PR #32 / `005-elf32-dependency-loading` on the single branch `m4-elf32-dependency-loading`.

1. Gate T002 — direct dependencies with provider-identity graph reuse.
   - Status: IMPLEMENTED; exact implementation-head CI #185 PENDING at `8a852f6ec328aa3acf8add6d13f0b7ec51f74482`.
   - Implemented behavior: root direct `DT_NEEDED` resolution through unchanged acquisition resolver; ordered/repeated edges; provider-identity alias/dedup; automatic ET_DYN dependency placement/loading; ET_EXEC dependency rejection; equal-identity byte consistency; graph-wide object/occurrence/image ceilings; transactional rollback of all graph-owned mappings.
   - Focused tests added for ordered direct dependencies, repeated/alias identity reuse, identity/image mismatch, ET_EXEC rejection, provider errors, object limits, total-image budget, and rollback.
   - Exact next action: recheck CI #185. If all three jobs PASS, mark T002 DONE and advance to T003. If any job FAILS, inspect the failing step/test and fix only that regression.
   - DoD: Linux A32 smoke PASS, Android x86_64 address-space probe PASS, Android arm64-v8a cross-build PASS on `8a852f6e...`.

2. After T002 PASS, implement T003 — bounded recursion, cycles, shared transitive objects, max-depth enforcement, and reverse-order aggregate rollback.
3. T004 integrates the pinned real ARM32 fixture through the completed graph API.
4. T005 converges docs/state and runs the final exact-head merge gate.

Hardware evidence remains BLOCKED until an accessible Android environment is available. Project license remains BLOCKED on maintainer choice.

Branch hygiene: keep the entire feature on `m4-elf32-dependency-loading`; do not create task-by-task or reconciliation branches.
