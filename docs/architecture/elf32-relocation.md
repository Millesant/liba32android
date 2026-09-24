# ELF32 ARM relocation application

Status: current through feature 009; eager main-REL and PLT JUMP_SLOT application validated

## Boundary

`elf32_relocation` is the guest-memory-mutating dynamic-linker layer above the loaded `Elf32DependencyGraph`, validated linker metadata, bounded linker-string access, and graph-local symbol lookup.

It consumes only logical 32-bit guest state:

- `memory::GuestMemory`;
- one immutable loaded dependency graph for the duration of a plan/resolve/apply call;
- one relocating object index;
- either the object's validated main `rel_table` descriptor or its validated `plt_rel_table` descriptor;
- bounded symbol/hash/string/scope options.

The public APIs keep the two table contracts separate:

- `build_elf32_rel_relocation_plan` / `resolve_elf32_rel_relocation_references` / `apply_elf32_rel_relocations` operate only on the main `DT_REL` table;
- `build_elf32_plt_rel_relocation_plan` / `resolve_elf32_plt_rel_relocation_references` / `apply_elf32_plt_rel_relocations` operate only on the feature-008 PLT REL descriptor.

Internal decoding, reference validation, and transactional-write mechanics are shared so the two paths do not drift, but each table has its own accepted relocation-type policy.

The layer does not acquire dependencies, choose guest bases, implement pathname/namespace/global-group policy, perform lazy binding, execute guest code, or convert guest addresses into host pointers.

## Plan before mutation

Each 8-byte `Elf32_Rel` entry is decoded explicitly as little-endian `r_offset` and `r_info`:

```text
symbol_index = r_info >> 8
type         = r_info & 0xff
P            = checked_u32(load_bias + r_offset)
```

Planning is bounded by caller-selected `max_relocations`. Supported write-producing entries require a word-aligned readable target. The original 32-bit target word is captured before mutation and duplicate write targets within the selected table are rejected.

For main REL, the captured word is both rollback state and the implicit REL addend for formulas that use `A`.

For PLT `R_ARM_JUMP_SLOT`, the captured word is rollback state only. Eager JUMP_SLOT semantics never treat the in-place word as an addend.

Missing main or PLT descriptors independently succeed as empty work. Main and PLT tables are not combined into one implicit transaction.

## Reference resolution

Main `R_ARM_ABS32` / `R_ARM_GLOB_DAT` and PLT `R_ARM_JUMP_SLOT` all use the relocating object's dynamic-symbol index and exact string-table name, then feature-006 graph-local breadth-first lookup beginning at that object.

The accepted reference contract is shared:

- `STB_GLOBAL` or `STB_WEAK`;
- `STV_DEFAULT`;
- NOTYPE, OBJECT, or FUNC;
- ordinary section indexes plus `SHN_ABS`.

Protected requester semantics, hidden/internal/local references, TLS, IFUNC, COMMON, XINDEX, and versioned objects fail explicitly rather than being approximated.

A graph miss is an error for a strong reference. An unresolved weak reference becomes `S = 0` at relocation policy.

## Supported main-REL formulas

The bounded main table supports:

- `R_ARM_NONE` (0): no write;
- `R_ARM_ABS32` (2): `S + A` modulo 2^32;
- `R_ARM_GLOB_DAT` (21): `S`;
- `R_ARM_RELATIVE` (23): `B + A` modulo 2^32, with symbol index zero required.

For ARM `R_ARM_GLOB_DAT`, the runtime follows Android bionic behavior and ignores the in-place REL addend. Synthetic coverage uses a non-zero original word to lock that compatibility choice.

## Supported PLT formula

The PLT table currently accepts only:

- `R_ARM_JUMP_SLOT` (22): eager `S`.

AAELF32 defines the REL-form JUMP_SLOT addend as zero and resolves the slot to the symbol address. Android bionic likewise uses the non-REL-addend path and writes the resolved symbol address.

The runtime therefore resolves every supported slot during the explicit PLT apply call and writes `S` directly. The original slot word is preserved solely for rollback. No lazy resolver state, `DT_PLTGOT` protocol, or first-call execution behavior is introduced.

## Transaction and rollback contract

Application has two phases.

First, the complete selected table is decoded and every semantic condition is checked:

- count and descriptor invariants;
- target access/alignment/uniqueness;
- symbol/string/hash reads;
- graph lookup or weak-zero policy;
- relocation-specific formula constraints;
- every final 32-bit word.

No relocation write occurs before that phase completes.

Second, writes occur through `GuestMemory::write` in selected-table order. If a later write fails, earlier successful relocation-owned writes are restored in reverse order from captured original words. Restoration is reread and verified.

A successful rollback returns the primary target-write failure with no published successful application writes. If restoration itself fails, the result is `RollbackFailed`, preserves the primary `TargetWriteFailed`, and identifies the rollback-failing relocation when available.

The layer never changes guest page permissions to make a relocation succeed.

## Address, ownership, and compatibility invariants

- Guest VAs and relocation results are logical 32-bit values, never host pointers.
- All guest bytes are accessed through `GuestMemory`.
- The graph is borrowed and immutable for one plan/resolve/apply call.
- Plans own only decoded metadata plus original/final 32-bit words and bounded reference state.
- Main and PLT APIs remain separate even though implementation helpers are shared.
- Failure before the write phase leaves relocation targets unchanged.
- Main-`DT_REL` feature-007 formulas remain unchanged by feature 009.

## Validation evidence

### Main REL — feature 007

Feature 007 established the main table path:

- T001 plan: CI #210 / run `35850236188`;
- T002 reference resolution: CI #212 / run `35889244367`;
- T003 transactional apply/rollback: CI #214 / run `35890660951`;
- T004 pinned real GLOB_DAT application: CI #216 / run `35891830738`;
- T005 final convergence gate: CI #218 / run `35918899544` at `8efe792cfa58a3f34e02dfe0c8bb01fbc3949766`.

The pinned loader fixture has two real main-`DT_REL` `R_ARM_GLOB_DAT` entries for `fixture_bss` and `fixture_data`; application writes the graph-resolved guest values while preserving all other readable segment bytes and mapping permissions.

### Eager PLT JUMP_SLOT — feature 009

T001 read-only PLT planning/reference resolution PASSed CI #226 / run `35938429972` at `fe12b6de747884a18d1214f564559d94937d8974`. PLT accepts only JUMP_SLOT while the main table continues to reject type 22.

T002 transactional eager application PASSed CI #227 / run `35938885569` at `666a15ab2edaebdfa3c0f6817dca30e2e2e7a931`. Synthetic coverage proves non-zero original slot words are ignored semantically, unresolved weak references write zero, strong misses fail before mutation, and late write/rollback failures retain the feature-007 transaction contract.

T003 real ARMv7 integration PASSed CI #228 / run `35939575947` at `815386149732201ce5b64e1b5ad207079491eb80`. The pinned NDK r27d / API 26 build produces byte-identical freestanding provider/consumer DSOs. `readelf` confirms the consumer declares `DT_NEEDED liba32android_jump_slot_provider.so` plus `R_ARM_JUMP_SLOT fixture_import`. The dependency loader builds a two-object graph, graph-local lookup resolves `fixture_import` to the provider, and eager PLT application rewrites the real slot to that logical guest value without executing ARM code. Mapping permissions and every readable segment byte outside the slot remain unchanged.

The run uploaded artifact `arm32-loader-fixture-815386149732201ce5b64e1b5ad207079491eb80`, ID `10783439676`, digest `sha256:4a68646d281cb35ceb69586388acd5ce0bbb5e5f316ecd285b1b8c4574bffee7`, containing the provider/consumer pair and JUMP_SLOT evidence alongside the existing ARM32 fixture evidence.

## Deliberate limits

The current relocation layer still does not implement:

- lazy PLT binding, resolver trampolines, or `DT_PLTGOT` runtime protocol;
- an atomic combined main-REL + PLT-REL transaction;
- REL32, COPY, instruction relocations, RELA, RELR, Android packed relocations, or APS2;
- symbol-version matching or requester-specific protected/`DT_SYMBOLIC` self-binding;
- Android namespaces, preloads, global groups, or process-wide interposition policy;
- TLS relocations/addressing or GNU IFUNC execution;
- RELRO;
- text-relocation permission broadening;
- constructors/destructors, `dlopen`, `dlsym`, unload, or guest execution.

Those remain separate contracts rather than implicit compatibility behavior.
