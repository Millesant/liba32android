# Requirements — ELF32 Symbol Resolution

Status: implementation validated through T004; T005 convergence and final exact-head gate active

## Goal

Add the first bounded ELF32 dynamic-symbol lookup layer above the existing validated linker metadata and recursive dependency graph.

The feature must:

- safely interpret `DT_HASH` and `DT_GNU_HASH` metadata from loaded ARM ELF32 objects;
- derive a bounded dynamic-symbol extent without section headers;
- look up externally visible defined symbols by exact byte name;
- compute a logical 32-bit guest value for supported non-TLS definitions;
- build deterministic graph-local lookup scope from dependency edges rather than relying on graph object-vector order;
- preserve existing mapping, dependency-provider, and graph-loading boundaries.

This feature stops before relocation writes, PLT/JMPREL execution, TLS address calculation, symbol version matching, Android namespace/global-group policy, constructors, unload, or guest execution.

## Scope

- Extend validated linker metadata with rebased descriptors for `DT_HASH` and `DT_GNU_HASH`.
- Support bounded SysV ELF hash-table parsing.
- Support bounded GNU hash-table parsing for ELF32, including bloom/bucket/chain layout.
- Derive the dynamic-symbol count:
  - from SysV `nchain` when `DT_HASH` is available;
  - from bounded GNU bucket/chain termination when GNU hash is the only supported hash.
- Validate the complete bounded `Elf32_Sym` range implied by the chosen symbol count.
- Parse individual 16-byte ELF32 symbol entries through `GuestMemory`.
- Compare candidate names through the existing bounded string-table reader.
- Support `STB_GLOBAL` and `STB_WEAK` externally visible definitions.
- Treat `STV_DEFAULT` and `STV_PROTECTED` definitions as externally visible; do not export `STV_HIDDEN` or `STV_INTERNAL`.
- Support normal defined symbols and `SHN_ABS`.
- Build a graph-local breadth-first scope from a selected object and its ordered dependency edges, with identity/cycle reuse already represented by the graph.
- Return the first eligible definition in that deterministic scope.
- Expose explicit resource ceilings for symbols, hash buckets, GNU bloom words, scope objects, and symbol-name bytes.
- Integrate the pinned real ARM32 fixture and resolve its exported `fixture_add`, `fixture_data`, and `fixture_bss` symbols through GNU hash.

## Non-goals

- Applying ARM relocations or writing resolved values into guest memory.
- PLT/GOT/JMPREL processing or lazy binding.
- Android/bionic filesystem lookup, namespaces, global groups, `LD_PRELOAD`, `RTLD_GLOBAL`, `RTLD_LOCAL`, or target-SDK compatibility policy.
- Process-wide link-map lifetime across independent graph-loading calls.
- Symbol versioning (`DT_VERSYM`, `DT_VERDEF`, `DT_VERNEED`) in this first feature.
- `DT_SYMBOLIC` / `DF_SYMBOLIC` requester-specific relocation semantics.
- TLS address calculation or TLS relocation semantics.
- GNU IFUNC execution/resolution.
- `dlopen`, `dlsym`, unload, constructors/destructors, RELRO, or guest execution.
- Using section headers to recover dynamic-symbol information.
- Treating guest addresses as host pointers.

## Requirements

### R1 — Layer boundary

Add a separate symbol-lookup layer above validated linker metadata/string access and above the loaded dependency graph.

The existing layers retain their current responsibilities:

- `elf32_dynamic` parses raw dynamic entries only;
- `elf32_linker_metadata` validates/rebases structural linker metadata;
- `elf32_linker_strings` provides bounded string-table reads;
- `elf32_dependency_loader` owns mapping, dependency graph identity, and rollback.

The symbol layer may consume these results but must not move pathname policy, mapping ownership, or relocation writes into them.

### R2 — Hash metadata descriptors

Recognize `DT_HASH` and `DT_GNU_HASH` as singleton pointer-like linker metadata.

For each present tag:

- add the object load bias exactly once with checked 32-bit arithmetic;
- validate that the fixed hash header is readable through `GuestMemory`;
- retain only a guest-address descriptor at metadata stage;
- defer variable-sized hash-array interpretation to the symbol layer.

Duplicate occurrences of either recognized hash tag are malformed.

A hash tag may be absent. Symbol lookup requires at least one supported hash when a searchable symbol table is present.

### R3 — Required symbol/string metadata

Searchable dynamic symbols require:

- validated `DT_SYMTAB` / `DT_SYMENT == 16`;
- validated `DT_STRTAB` / `DT_STRSZ`;
- at least one supported hash descriptor.

An object with no symbol table is simply not searchable.

An object with a symbol table but missing string-table or supported-hash metadata must fail symbol-index construction rather than being silently skipped as though it had no symbols.

### R4 — SysV hash validation and count

For `DT_HASH`, parse the ELF hash table as 32-bit little-endian words:

1. `nbucket`;
2. `nchain`;
3. `nbucket` bucket indexes;
4. `nchain` chain indexes.

Requirements:

- `nbucket > 0`;
- `nchain > 0` because dynamic symbol index 0 exists;
- counts must fit caller ceilings and checked guest-address arithmetic;
- the complete bounded table must be readable;
- every nonzero bucket/chain symbol index must be less than `nchain`.

The SysV dynamic-symbol count is exactly `nchain`.

Malformed chains must never permit an unbounded loop; lookup is additionally bounded by `nchain`.

### R5 — GNU hash validation and count

For ELF32 `DT_GNU_HASH`, parse:

- four 32-bit header words: bucket count, symbol offset, bloom-word count, bloom shift;
- a bloom filter of 32-bit words;
- 32-bit bucket indexes;
- 32-bit chain hash values beginning at the symbol offset.

Requirements:

- bucket count is nonzero;
- bloom-word count is nonzero and a power of two;
- bucket and bloom counts fit caller ceilings;
- every nonzero bucket index is at least the GNU symbol offset;
- checked address arithmetic is used for every derived array address;
- chain traversal is bounded by the symbol ceiling;
- a GNU chain must terminate with its low bit set before the bound is exhausted.

When GNU hash is the only supported hash, derive the symbol count by:

1. finding the maximum nonzero bucket start;
2. scanning that bucket's chain from `bucket - symoffset` until its terminating low bit;
3. setting count to one plus the final symbol index;
4. using `symoffset` as the count when all buckets are zero.

When SysV hash is also present, `nchain` is the authoritative symbol count; GNU bucket/chain indexes must remain inside that count.

### R6 — Symbol-table range and entry decoding

After count derivation:

- require `symbol_count <= max_symbols`;
- validate `symbol_count * 16` with checked arithmetic;
- require the full implied symbol-table range to be readable through `GuestMemory`.

Decode an ELF32 symbol entry explicitly as:

- `st_name: uint32`;
- `st_value: uint32`;
- `st_size: uint32`;
- `st_info: uint8`;
- `st_other: uint8`;
- `st_shndx: uint16`.

Do not rely on host structure packing, host endianness, or host pointers.

Symbol index 0 remains the reserved undefined entry.

### R7 — Name lookup

Lookup requests use exact non-empty byte strings.

Hashing must match the selected format:

- SysV ELF hash for `DT_HASH`;
- GNU DJB-style 32-bit hash for `DT_GNU_HASH`.

When both hash formats are present, GNU hash is the preferred lookup path after both descriptors have passed structural validation. A GNU miss is a real miss and does not fall back to SysV to mask malformed/inconsistent GNU data.

Candidate `st_name` offsets must lie inside the validated string table. Candidate names are read through `read_elf32_string_table_entry` with the caller's explicit byte ceiling and compared exactly.

### R8 — Eligible external definitions

A symbol is an eligible external definition only when all of the following hold:

- binding is `STB_GLOBAL` or `STB_WEAK`;
- section index is not `SHN_UNDEF`;
- visibility is `STV_DEFAULT` or `STV_PROTECTED`;
- its type/value semantics are supported by this feature.

`STB_LOCAL` never satisfies external lookup.

`STV_HIDDEN` and `STV_INTERNAL` definitions are not externally visible and are skipped as misses.

The first eligible definition in lookup order wins even if it is weak; this feature does not continue scanning for a later strong definition. Unresolved weak-reference-to-zero behavior belongs to the future relocation layer.

### R9 — Value semantics

For a selected supported definition:

- `SHN_ABS`: resolved guest value is the raw `st_value`;
- ordinary allocated definitions: resolved guest value is `load_bias + st_value` with checked 32-bit arithmetic.

Return both raw symbol metadata and the resolved guest value.

The result must identify the defining object index and symbol index.

### R10 — Unsupported symbol forms

The first feature must fail explicitly, rather than invent semantics, when a name-matching candidate requires unsupported behavior, including:

- `SHN_XINDEX`;
- `SHN_COMMON`;
- `STT_TLS`;
- GNU IFUNC or another OS/processor-specific symbol type that requires execution or ABI-specific handling.

Unknown/nonstandard binding, type, or visibility values encountered on a matching candidate must produce an explicit unsupported/malformed error rather than being treated as a valid definition.

Non-matching entries need not fail solely because they use an unsupported type.

### R11 — Versioning boundary

If an object declares GNU/SysV symbol-version metadata relevant to dynamic lookup (`DT_VERSYM`, `DT_VERDEF`, or `DT_VERNEED` families), graph-level symbol resolution for that object must report explicit unsupported-versioning status in this first feature.

Do not silently ignore version tables and return a potentially wrong same-name definition.

Version-aware lookup is deferred.

### R12 — Graph-local scope

The graph result from feature 005 intentionally assigns no symbol-scope semantics to object-vector order. Therefore this feature must derive lookup scope from dependency edges.

For a selected start object:

1. visit the start object;
2. visit its direct dependency targets in stored `DT_NEEDED` edge order;
3. then visit transitive dependencies breadth-first, preserving each object's edge order;
4. visit each object index at most once.

This produces a deterministic graph-local breadth-first closure and terminates cycles/shared dependencies.

The scope is local to the supplied graph. It does not model Android global groups, namespaces, preloads, or process-wide interposition.

### R13 — Graph lookup behavior

Graph lookup searches each object in the derived scope order and returns the first eligible definition.

If an earlier searchable object is structurally malformed or uses unsupported versioning required for lookup, return that failure rather than skipping it and selecting a later definition.

Objects with no symbol table are skipped.

A successful graph lookup does not mutate guest memory, graph state, mappings, or provider state.

### R14 — Explicit resource bounds

Every lookup/index operation must use finite caller-selected ceilings for at least:

- maximum dynamic symbols per object;
- maximum SysV/GNU hash buckets;
- maximum GNU bloom words;
- maximum graph-scope objects;
- maximum symbol-name payload bytes.

All count/size/address multiplication and addition uses checked arithmetic.

No malformed hash table may cause unbounded chain traversal, recursion, allocation, or guest reads.

### R15 — Error/result contract

Distinguish at least:

- missing string table;
- missing symbol table;
- missing supported hash table;
- duplicate hash metadata;
- hash address/range/read failure;
- invalid SysV header/count/index;
- invalid GNU header/bloom/bucket/unterminated chain;
- symbol-count/limit overflow;
- symbol-table read/range failure;
- string/name failure;
- symbol not found;
- unsupported symbol binding/type/visibility/section form;
- unsupported versioning;
- resolved-value overflow;
- invalid graph start/scope limit.

Failure must not return a successful partial symbol result.

### R16 — Compatibility and isolation

Existing public contracts remain source-compatible except for additive metadata/result fields and new APIs.

Do not change:

- explicit-base `load_elf32`;
- non-mutating dynamic placement;
- dependency-provider request semantics;
- dependency graph object/edge structure;
- logical 32-bit guest-address invariants.

No host pointer appears in symbol APIs/results.

### R17 — Validation discipline

Completion requires:

- focused SysV hash success/malformed/limit coverage;
- focused GNU hash success/malformed/limit coverage;
- exact-name and collision coverage;
- binding/visibility/undefined/absolute/unsupported-form coverage;
- graph BFS order with repeated/cyclic/shared dependencies;
- weak-first behavior;
- unchanged guest bytes/mappings after success/failure;
- pinned real ARM32 fixture GNU-hash lookups for `fixture_add`, `fixture_data`, and `fixture_bss`;
- all neighboring ELF/linker/dependency tests green;
- Linux CTest, Android x86_64 probe cross-build, and Android arm64-v8a cross-build PASS at final feature head.

No Android device execution is required for this host-side lookup feature.

## Acceptance Criteria

- AC1: Valid SysV hash metadata produces the exact `nchain` dynamic-symbol count.
- AC2: Valid GNU-only hash metadata derives a finite exact symbol count from bounded bucket/chain termination.
- AC3: Malformed/oversized hash structures fail deterministically without out-of-range guest reads or mutation.
- AC4: Exact-name per-object lookup succeeds through both SysV and GNU hash paths.
- AC5: Hash collisions follow bounded chains and compare exact symbol names.
- AC6: Local, undefined, hidden, and internal symbols do not satisfy external lookup.
- AC7: Global/weak default/protected definitions are eligible; the first eligible definition wins.
- AC8: `SHN_ABS` returns raw `st_value`; ordinary definitions return checked `load_bias + st_value`.
- AC9: TLS/common/XINDEX/IFUNC-or-other explicitly unsupported matching forms fail explicitly.
- AC10: Versioned objects are rejected explicitly rather than silently matched by name only.
- AC11: Graph scope is breadth-first from the selected object, preserves dependency-edge order, visits shared/cyclic objects once, and does not depend on object-vector discovery order.
- AC12: Graph lookup returns defining object index, symbol index, raw metadata, and resolved guest value without mutation.
- AC13: Resource ceilings bound symbols, buckets, bloom words, scope objects, and names.
- AC14: The pinned real ARM32 fixture resolves `fixture_add`, `fixture_data`, and `fixture_bss` through GNU hash; `fixture_data` reads as `0x12345678` and `fixture_bss` reads as zero at the resolved guest values.
- AC15: Existing loader/dynamic/metadata/string/dependency tests remain PASS.
- AC16: Final exact-head Linux CTest, Android x86_64 probe, and Android arm64-v8a cross-build PASS.

## Invariants

- Guest symbol values are logical 32-bit guest addresses/values, never host pointers.
- Hash/symbol/string reads go only through `GuestMemory`.
- Graph object-vector order remains discovery data, not implicit symbol scope.
- Dependency-edge order is the only graph ordering input to local BFS scope.
- Lookup is read-only.
- Unsupported versioning/TLS/IFUNC semantics are explicit, not silently approximated.
- Relocation writes remain downstream.

## Security / Performance Constraints

- Every variable-size table is bounded before allocation/traversal.
- GNU/SysV chain walks have hard finite limits.
- Hash indices are validated before symbol-table access.
- String offsets are validated before string reads.
- Whole-symbol-table range validation is bounded by `max_symbols`.
- Expected lookup cost is hash-directed rather than linear scan over all symbols.
- Graph traversal is O(objects + edges) within explicit scope bounds.

## Evidence / Reference Basis

The behavioral contract is based on:

- ELF Object File Format / gABI symbol-table and `DT_HASH` semantics: binding, visibility, `SHN_UNDEF`, `SHN_ABS`, TLS/common semantics, and SysV hash `nchain` count.
- GNU hash layout as implemented by GNU-compatible linkers and Android bionic: 4-word header, power-of-two bloom-word count, buckets, low-bit-terminated chains, GNU hash function.
- Android bionic's documented API-22+ breadth-first dependency/local-group ordering as compatibility evidence for choosing breadth-first graph-local scope. This project does not yet implement Android global groups/namespaces.

The project remains intentionally stricter than a production Android linker where unsupported semantics would otherwise risk silently incorrect resolution.

## Open Questions

No unresolved question blocks T001.

Later features must define:

- symbol version matching and default/non-default versions;
- requester-specific `DT_SYMBOLIC` / protected self-binding during relocations;
- process-wide/global-group/namespace interposition;
- TLS symbol address calculation;
- GNU IFUNC execution;
- ARM relocation semantics and weak-reference-to-zero application.
