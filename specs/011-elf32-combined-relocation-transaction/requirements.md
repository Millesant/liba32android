# Requirements — ELF32 combined main+PLT relocation transaction

Status: DONE — exact-head implementation CI PASSed at `600fad8edc3ac2a8f64fc2607e088b263adca902`

## Goal
Provide an explicit per-object transaction spanning the supported main DT_REL and eager PLT REL tables without changing either table's formulas.

## Accepted requirements
- Both tables plan/resolve/validate and compute final words before any write.
- max_relocations remains an independent per-table bound.
- Cross-table duplicate write targets are rejected before mutation.
- Writes run main-table order then PLT-table order.
- Later failures roll back in reverse across both tables and identify source table plus relocation index.
- Existing single-table APIs remain source-compatible and semantically unchanged.
- No lazy binding, new relocation families, permission changes, or process-wide linker policy.

## Validation
Linux check `107911342972`, Android arm64 check `107911343141`, and Android x86_64 check `107911343155` PASSed on the implementation revision.
