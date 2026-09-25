# Proposal — ELF32 combined main+PLT relocation transaction

## Intent
Add one opt-in per-object transaction across the already-supported main DT_REL and eager PLT REL paths.

## Why now
The two tables already share semantics and rollback mechanics, but separate calls can commit main writes before a later PLT failure. Combining preparation and rollback is a bounded continuation that adds no relocation family or linker policy.

## Contract
Both tables prepare fully before mutation; writes are main then PLT; cross-table duplicate targets fail prewrite; rollback runs in reverse across both tables; existing single-table APIs remain independent.

## Non-goals
No lazy binding, new relocation forms, namespace/interposition policy, TLS, IFUNC, permission broadening, RELRO folding, unload semantics, or guest execution.
