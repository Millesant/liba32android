# Next Work

Repository integration remains on `bleeding` through PR #31 / runtime commit `2c3be26c05dff81be1f81c9565df5906c521a54c`. Active feature work is `005-elf32-dependency-loading` on the single branch `m4-elf32-dependency-loading`.

1. Implement T001 — transactional root-object graph loader.
   - Status: TODO; spec readiness check PASS by inspection, implementation/tests NOT RUN.
   - Relevant: `specs/005-elf32-dependency-loading/{requirements,design,tasks}.md`, new `src/elf/elf32_dependency_loader.{h,cpp}`, CMake, focused tests.
   - Required behavior: owned non-empty root identity/image; explicit graph limits; dependency-free ET_EXEC root loading; ET_DYN root automatic placement followed by exact-base `load_elf32`; existing dynamic → metadata → strings pipeline; missing PT_DYNAMIC succeeds empty; post-load pipeline failure rolls back only root mappings.
   - Compatibility: do not modify `elf32_dependency_resolver`, explicit-base loader semantics, provider lookup policy, or guest/host pointer boundaries.
   - Validation: focused new host test plus neighboring ELF loader/placement/dynamic/metadata/string tests; authoritative CI only after a coherent implementation commit.
   - DoD: T001 acceptance cases pass and durable state records exact evidence.

2. After T001, implement T002 — direct dependencies with provider-identity graph reuse.
   - Depends on: T001.
   - DoD: ordered/repeated edges, aliases, one mapping per identity, ET_EXEC dependency rejection, mismatch/resource failures and rollback validated.

3. Hardware evidence remains BLOCKED until an accessible Android environment is available: crash tombstone/backtrace and a materially different vendor/kernel sample.

4. Project license remains BLOCKED on maintainer choice before public release.

Branch hygiene: keep this entire feature on `m4-elf32-dependency-loading`; do not create task-by-task or reconciliation branches.
