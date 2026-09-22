# Next Work

Repository integration remains on `bleeding` through PR #31 / runtime commit `2c3be26c05dff81be1f81c9565df5906c521a54c`. Active feature work is PR #32 / `005-elf32-dependency-loading` on the single branch `m4-elf32-dependency-loading`.

1. Implement T002 — direct dependencies with provider-identity graph reuse.
   - Status: TODO. Predecessor T001 DONE; exact-head CI #182 PASS on `9a1d5bf4e97656724fdb7d649197af0d8cdd1ed8`.
   - Existing T001 behavior: transactional root ET_EXEC/ET_DYN loading, automatic placement, dynamic → metadata → strings pipeline, root-owned rollback, focused host coverage.
   - Required T002 behavior: resolve the root's complete ordered direct `DT_NEEDED` set through the unchanged acquisition resolver; preserve repeated edges; key objects by provider identity; automatically place/load each first-seen ET_DYN dependency once; reject ET_EXEC dependencies; alias different names to one identity; compare later equal-identity image bytes and fail on mismatch; enforce graph-wide object/occurrence/image/string bounds; roll back all graph-owned mappings on failure.
   - Compatibility: `elf32_dependency_resolver` remains acquisition-only; no recursive traversal yet (T003), no Android pathname policy, symbols, relocations, or execution.
   - Validation: ordered/repeated edges, identity aliases, one mapping per identity, identity/image mismatch, ET_EXEC dependency rejection, provider/resource errors, and aggregate rollback.
   - DoD: focused T002 host tests PASS and authoritative exact-head CI records all three jobs PASS.

2. After T002, implement T003 — bounded recursion, cycles, and aggregate rollback.
3. T004 integrates the pinned real ARM32 fixture through the graph API.
4. T005 converges docs/state and runs the final exact-head merge gate.

Hardware evidence remains BLOCKED until an accessible Android environment is available. Project license remains BLOCKED on maintainer choice.

Branch hygiene: keep the entire feature on `m4-elf32-dependency-loading`; do not create task-by-task or reconciliation branches.
