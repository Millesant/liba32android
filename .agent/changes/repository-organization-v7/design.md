# Repository organization v7

This change is intentionally behavior-preserving. It converts project workflow state to the v7 control-plane model and removes structural maintenance debt without widening runtime scope.

## Workflow migration

`.agent/project.toml` becomes the project identity record. `.agent/specs/` contains concise accepted current contracts. The existing numbered `specs/` packages are retained as historical feature-era requirements/design/tasks records; they are not copied into current truth or rewritten retroactively. Active substantial work remains under `.agent/changes/<change-id>/`.

## Build organization

The existing root `CMakeLists.txt` is partitioned by moving its current declarations verbatim into included modules for dependencies, runtime target, Android probes, and tests. Because `include()` executes in the caller directory scope, target names, output locations, cache variables, test commands, and current build behavior remain unchanged.

## Runtime cleanup

A private `elf32_bytes.h` helper owns duplicated little-endian decode primitives and checked 32-bit guest-address addition. Call sites keep their existing error mapping and guest-memory behavior. No public header/API changes.

## Validation

The cleanup must pass the existing exact-head CI matrix. Feature 010's real RELRO test is part of that same gate; no prior historical green result is treated as validation of the cleanup revision.
