# Design — ELF32 ARM eager JUMP_SLOT relocations

Status: READY — readiness checked at base `bbad7db39e430ba87a373841e3ede954696c85c5`

## Context

Feature 007 established a bounded relocation pipeline:

```text
validated main REL descriptor
 -> byte-explicit plan
 -> bounded reference resolution
 -> precomputed transactional writes
```

Feature 008 added a separate validated `plt_rel_table` descriptor and deliberately stopped before relocation semantics.

Feature 009 reuses the proven pipeline for a second table source while keeping the public main/PLT contracts distinct.

## Public API shape

Add:

```cpp
inline constexpr std::uint8_t kRArmJumpSlot = 22;

Elf32RelocationPlanResult build_elf32_plt_rel_relocation_plan(...);
Elf32RelocationResolutionResult resolve_elf32_plt_rel_relocation_references(...);
Elf32RelocationApplyResult apply_elf32_plt_rel_relocations(...);
```

Existing result types are sufficient because a PLT entry has the same decoded REL fields, original rollback word, resolved reference, and transactional write record.

No table-kind field is added unless implementation requires one for unambiguous diagnostics; the called API already establishes table identity.

## Internal factoring

Avoid duplicating the entire feature-007 implementation.

Introduce narrow internal helpers parameterized by:

- selected `Elf32RelTableMetadata` descriptor;
- allowed relocation-type policy;
- whether a relocation is symbol-bearing;
- final-word calculation policy.

Recommended factoring:

```text
build_relocation_plan_for_table(...)
resolve_relocation_plan_references(...)
apply_resolved_relocations(...)
```

The existing main APIs call these helpers with the feature-007 type/formula policy. The new PLT APIs call them with a strict JUMP_SLOT-only policy.

The refactor must be behavior-preserving for main REL tests.

## PLT planning

`build_elf32_plt_rel_relocation_plan`:

1. validates object index;
2. returns empty success when `plt_rel_table` is absent;
3. requires finite `max_relocations` when present;
4. defensively checks 8-byte descriptor invariants;
5. decodes every REL entry byte-explicitly;
6. rejects every type except `R_ARM_JUMP_SLOT`;
7. computes checked `load_bias + r_offset`;
8. requires word alignment/readability;
9. snapshots the original word;
10. rejects duplicate target addresses.

The original word is rollback state only.

## Reference resolution

Both main and PLT symbol-bearing entries use one shared resolver over a prepared plan.

For JUMP_SLOT, symbol index zero is rejected. The same binding/visibility/type/section/version boundaries remain in force.

This preserves one graph-local lookup policy instead of letting PLT behavior fork from main relocation behavior.

## JUMP_SLOT application

For every resolved JUMP_SLOT:

```text
final_word = S
```

Do not read the original word as an addend. The planner already read it for rollback.

Unresolved weak references produce `S = 0`.

All final words are produced before the first write.

The existing write/rollback engine can be shared unchanged if it accepts the precomputed write vector.

## Real fixture design

Add:

```text
tests/fixtures/arm32_jump_slot_provider.c
tests/fixtures/arm32_jump_slot_consumer.c
tools/build_arm32_jump_slot_fixture.sh
```

The builder accepts `<ndk-root> <output-dir>` and produces:

- `liba32android_jump_slot_provider.so`
- `liba32android_jump_slot_consumer.so`

Provider exports:

```c
uint32_t fixture_import(uint32_t value);
```

Consumer calls that symbol from a default-visible exported function. The provider is linked into the consumer command line so LLD emits the exact `DT_NEEDED` SONAME and PLT JUMP_SLOT rather than leaving an unowned undefined symbol.

Both DSOs use:

- `armv7a-linux-androideabi26-clang`;
- `-shared -fPIC -ffreestanding -fno-stack-protector -nostdlib`;
- no build ID;
- `-z max-page-size=16384`;
- explicit SONAMEs.

CI builds the pair twice in separate directories and compares provider-to-provider and consumer-to-consumer bytes.

CI records `readelf -dW` and `readelf -rW` evidence and checks the provider SONAME appears as consumer `DT_NEEDED` plus at least one `R_ARM_JUMP_SLOT fixture_import` record.

## Real application test

Add one test executable accepting consumer and provider paths.

A provider implementation returns the provider bytes only for the exact consumer `DT_NEEDED` name and fails otherwise.

After `load_elf32_dependency_graph`:

- graph object 0 is consumer;
- graph object 1 is provider;
- consumer has one dependency edge to provider;
- consumer exposes `plt_rel_table`;
- feature-009 PLT planning reports JUMP_SLOT entries;
- graph symbol lookup for `fixture_import` resolves to provider object;
- eager PLT application writes that guest value into the real slot.

Snapshot mapped segments before apply and allow changes only in planned PLT target words. Mapping permissions remain identical.

No A32 execution is needed.

## CMake / CI plumbing

Add cache variables for the consumer/provider fixture paths when tests are enabled.

Build the real JUMP_SLOT test only when both paths are supplied; require both-or-neither to prevent partial configuration.

Linux CI builds/rebuilds the pair, passes both paths to CMake, executes the new test, records readelf/application evidence, and uploads both DSOs plus evidence.

Android jobs remain compile/probe regressions; they do not need the ARM32 fixture files.

## Failure semantics

Reuse existing relocation error enums where semantics already fit:

- invalid options/object/count/read/place/alignment/target/duplicate/type;
- resolution index/name/form/lookup errors;
- target write and rollback failures.

No new error is required solely because the table is PLT; the called API gives that context.

## Documentation obligations

Update:

- `elf32_relocation.h` comments for new APIs and JUMP_SLOT semantics;
- `docs/architecture/elf32-relocation.md`;
- linker-metadata docs only if the consumer boundary wording needs a small update;
- README/current project context/state at convergence.

## Alternatives

### Fold PLT entries into apply_elf32_rel_relocations

Rejected. Main and PLT tables have different accepted relocation sets and future lazy-binding policy; a single implicit API would erase an important contract boundary.

### Duplicate the feature-007 implementation

Rejected. Symbol/reference and rollback semantics must not drift between main and PLT paths.

### Implement lazy binding now

Rejected. It requires `DT_PLTGOT`, resolver entry state, first-call mutation/execution semantics, and lifetime/concurrency policy beyond this slice.

### Synthetic-only validation

Rejected. A small freestanding provider/consumer pair can prove actual NDK-generated dynamic tags, dependency acquisition, symbol lookup, and a real PLT slot without guest execution.

## Readiness

- Acceptance is observable and bounded.
- Required metadata and graph/symbol prerequisites are complete.
- External ABI/bionic semantics are resolved.
- No unresolved lazy-binding question blocks eager application.
- T001-T004 form an acyclic sequence.
- Required GitHub/CI capabilities are available.
