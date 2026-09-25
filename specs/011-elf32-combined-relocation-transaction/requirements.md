# Requirements — ELF32 combined main+PLT relocation transaction

Status: ACTIVE — implementation prepared; exact-head validation NOT RUN

## Goal
Provide an explicit per-object transaction spanning the supported main DT_REL and eager PLT REL tables without changing either table's formulas.

## Requirements
- Both tables plan/resolve/validate and compute final words before any write.
- max_relocations remains an independent per-table bound.
- Cross-table duplicate write targets are rejected before mutation.
- Writes run main-table order then PLT-table order.
- Later failures roll back in reverse across both tables and identify source table plus relocation index.
- Existing single-table APIs remain source-compatible and semantically unchanged.
- No lazy binding, new relocation families, permission changes, or process-wide linker policy.

## Acceptance
Unit coverage proves success order, PLT prewrite failure, duplicate rejection, cross-table rollback, and explicit rollback failure. Required exact-head CI must pass before closure.
