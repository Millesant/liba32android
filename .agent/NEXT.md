# Next Work

Repository integration remains on `bleeding` through PR #31 / runtime commit `2c3be26c05dff81be1f81c9565df5906c521a54c`. Active feature work is draft PR #32 / `005-elf32-dependency-loading` on the single branch `m4-elf32-dependency-loading`.

1. Implement T003 — bounded recursion, cycles, shared transitive objects, and aggregate rollback.
   - Status: TODO. T001 DONE / CI #182 PASS. T002 DONE / latest containing-head CI #187 PASS at `1d85b223fe8fb6ef4fd1c89806c17afd2811e219`.
   - Existing T002 behavior: root direct dependency resolution; ordered/repeated edges; provider-identity alias/dedup; automatic ET_DYN child placement/loading; ET_EXEC child rejection; identity/image consistency; direct graph resource limits; rollback.
   - Required T003 behavior: add Discovered/Loading/Loaded state; recursively process new dependency objects; deterministic depth-first traversal after each object's direct acquisition; terminate cycles by reusing Loading/Loaded identities; reuse shared transitive objects; enforce max depth and graph-wide object/occurrence/image budgets through recursion; reverse-order rollback across every graph-owned successful mapping on later transitive failure.
   - Required validation: A→B→C; A→B→A cycle with two mapped objects; A→B,C and B→C sharing; deterministic depth-limit failure; later transitive acquisition/parse/placement/load failure removes all graph-owned mappings while preserving unrelated preexisting mappings.
   - Compatibility: keep the acquisition resolver unchanged and occurrence-preserving; no Android pathname policy, symbols, relocations, TLS/RELRO, constructors, unload, or execution.
   - Exact next action: refactor the current direct-only graph loop into bounded recursive object processing without changing the public graph/result contract unless evidence requires it.
   - DoD: focused T003 host tests PASS and authoritative exact-head CI records Linux A32 smoke, Android x86_64 probe, and Android arm64-v8a cross-build PASS.

2. T004 integrates the pinned real ARM32 fixture through the completed graph API.
3. T005 converges docs/state and runs the final exact-head merge gate.

Hardware evidence remains BLOCKED until an accessible Android environment is available. Project license remains BLOCKED on maintainer choice.

Branch hygiene: keep the entire feature on `m4-elf32-dependency-loading`; do not create task-by-task or reconciliation branches.
