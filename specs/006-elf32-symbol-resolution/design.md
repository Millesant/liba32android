# Design — ELF32 Symbol Resolution

## Context

The runtime now has all preconditions needed for bounded symbol lookup without collapsing existing boundaries:

- `elf32_linker_metadata` validates/rebases STRTAB and SYMTAB descriptors but intentionally does not know whole-symbol extent;
- `elf32_linker_strings` can read one bounded string-table entry through `GuestMemory`;
- `elf32_dependency_loader` returns an owned recursive object graph with load bias, linker metadata, exact dependency edges, and source ownership;
- the pinned ARMv7 fixture exports `fixture_add`, `fixture_data`, and `fixture_bss` and contains `DT_GNU_HASH`.

Feature 001 explicitly deferred `DT_HASH`/`DT_GNU_HASH` because symbol count could not be established safely without them. Feature 006 closes that boundary.

## Chosen Approach

Add a separate `elf32_symbol_lookup` component.

The feature is split into five slices:

1. extend validated linker metadata with hash descriptors and build a bounded per-object symbol index;
2. perform exact-name per-object hash lookup and return supported guest values;
3. derive deterministic breadth-first graph scope and perform first-definition graph lookup;
4. integrate the real GNU-hash ARM32 fixture;
5. converge documentation/state and run final exact-head CI.

The dependency graph itself is not modified to cache symbol indices. Symbol lookup remains a read-only consumer so graph loading does not become dependent on future symbol semantics.

## Architecture

```text
loaded object / dependency graph
        |
        +--> load result (load_bias)
        +--> validated STRTAB/SYMTAB
        +--> validated DT_HASH/DT_GNU_HASH descriptors
        |
        v
elf32_symbol_lookup
        |
        +--> build bounded object symbol index
        |      |
        |      +--> SysV: nchain => symbol_count
        |      |
        |      +--> GNU-only:
        |             max bucket -> bounded terminal chain
        |             => symbol_count
        |
        +--> hash-directed per-object lookup
        |      |
        |      +--> candidate Elf32_Sym decode
        |      +--> bounded STRTAB name read
        |      +--> binding/visibility/section/type filter
        |      +--> logical guest value calculation
        |
        +--> graph-local scope builder
               |
               +--> breadth-first dependency-edge walk
               +--> visit each object once
               |
               v
        first eligible object definition
```

No path performs relocation writes.

## Metadata Extension

`Elf32CollectedLinkerMetadata` gains optional raw pointer values for:

- SysV hash (`DT_HASH == 4`);
- GNU hash (`DT_GNU_HASH == 0x6ffffef5`).

`Elf32LinkerMetadata` gains rebased guest descriptors:

```cpp
struct Elf32HashTableMetadata {
    std::uint32_t guest_address{};
};

struct Elf32GnuHashTableMetadata {
    std::uint32_t guest_address{};
};
```

The metadata layer validates only the fixed header readability:

- SysV: 8 bytes;
- GNU: 16 bytes.

It does not trust or traverse variable counts. That belongs to `elf32_symbol_lookup`, where caller limits are available.

Hash tags are singleton metadata and duplicate occurrences fail consistently with existing singleton handling.

## Public Symbol-Lookup Shape

Intended API shape:

```cpp
enum class Elf32SymbolHashKind : std::uint8_t {
    SysV,
    Gnu,
};

struct Elf32SymbolLookupOptions {
    std::uint32_t max_symbols{};
    std::uint32_t max_hash_buckets{};
    std::uint32_t max_gnu_bloom_words{};
    std::uint32_t max_scope_objects{};
    std::uint32_t max_name_bytes{};
};

struct Elf32SymbolIndex {
    std::uint32_t symbol_count{};
    std::optional<Elf32SysvHashIndex> sysv_hash;
    std::optional<Elf32GnuHashIndex> gnu_hash;
};

struct Elf32Symbol {
    std::uint32_t name_offset{};
    std::uint32_t value{};
    std::uint32_t size{};
    std::uint8_t binding{};
    std::uint8_t type{};
    std::uint8_t visibility{};
    std::uint16_t section_index{};
};

struct Elf32ResolvedSymbol {
    std::size_t object_index{};
    std::uint32_t symbol_index{};
    std::string name;
    Elf32Symbol symbol;
    std::uint32_t guest_value{};
};

Elf32SymbolIndexResult build_elf32_symbol_index(
    const memory::GuestMemory& memory,
    const Elf32LinkerMetadata& metadata,
    const Elf32SymbolLookupOptions& options);

Elf32ObjectSymbolLookupResult lookup_elf32_symbol(
    const memory::GuestMemory& memory,
    std::uint32_t load_bias,
    const Elf32LinkerMetadata& metadata,
    const Elf32SymbolIndex& index,
    std::string_view name,
    const Elf32SymbolLookupOptions& options);

Elf32GraphSymbolLookupResult lookup_elf32_graph_symbol(
    const memory::GuestMemory& memory,
    const Elf32DependencyGraph& graph,
    std::size_t start_object,
    std::string_view name,
    const Elf32SymbolLookupOptions& options);
```

Exact names may change during implementation, but ownership and semantic responsibilities should not.

The graph-level function may build bounded object indices lazily. A future cache can be added without changing lookup results.

## SysV Hash Index

Layout from `DT_HASH`:

```text
u32 nbucket
u32 nchain
u32 buckets[nbucket]
u32 chains[nchain]
```

Index construction:

1. read the 8-byte header;
2. reject zero bucket/count and caller-limit violations;
3. compute bucket/chain addresses with checked 64-bit arithmetic;
4. require the complete table readable;
5. validate every nonzero bucket and chain value is < `nchain`;
6. set `symbol_count = nchain`.

Lookup:

1. compute SysV ELF hash;
2. select `buckets[hash % nbucket]`;
3. follow chain indexes until index 0;
4. reject out-of-range indexes and cap hops at `nchain`;
5. decode candidate, read bounded name, compare exact bytes.

Index 0 is the hash-chain terminator and reserved undefined symbol.

## GNU Hash Index

ELF32 GNU hash layout:

```text
u32 nbuckets
u32 symoffset
u32 bloom_words
u32 bloom_shift
u32 bloom[bloom_words]     // ELF32 words
u32 buckets[nbuckets]
u32 chains[]               // symbol symoffset + i
```

Validation:

- `nbuckets > 0`;
- `bloom_words` is nonzero, power of two, and <= configured limit;
- `nbuckets <= max_hash_buckets`;
- all derived addresses are checked;
- every nonzero bucket is >= `symoffset`;
- chain reads stay below `max_symbols`.

### GNU-only symbol count

GNU hash has no explicit full symbol count.

For a GNU-only object:

1. read all bounded buckets;
2. find the maximum nonzero symbol index;
3. if none exists, use `symoffset` as count;
4. otherwise read chain words from `max_bucket - symoffset`;
5. advance the symbol index until a chain value with low bit 1 is observed;
6. count is final symbol index + 1;
7. fail if termination is not observed before `max_symbols`.

The GNU layout groups each bucket's symbols contiguously in dynsym order, so the bucket with the greatest start index owns the final chain group.

### Both hash styles

If SysV hash exists, `nchain` is the full symbol count.

GNU hash is still parsed/validated and preferred for name lookup. Every GNU bucket and chain index used must remain < the SysV count.

Do not fall back to SysV after a valid GNU lookup miss. Doing so could hide inconsistent/malformed GNU metadata.

## Symbol-Table Validation

Once `symbol_count` is known:

- require `symbol_count <= max_symbols`;
- compute bytes as `uint64_t(symbol_count) * 16`;
- require the range to fit the 32-bit guest address space;
- validate the complete range readable in bounded chunks.

Symbol decoding is byte-explicit and little-endian; no host `Elf32_Sym` reinterpret cast is used.

## Candidate Name Reads

The existing `read_elf32_string_table_entry` function is the only string-table materialization primitive.

Candidate `st_name` is rejected if outside STRTAB.

Each name read uses:

```cpp
Elf32LinkerStringOptions{
    .max_string_bytes = options.max_name_bytes,
}
```

This retains one bounded string implementation and its existing overflow/read/termination behavior.

## Eligibility and Value Semantics

Decode:

```text
binding    = st_info >> 4
type       = st_info & 0x0f
visibility = st_other & 0x03
```

External eligibility:

- binding: GLOBAL or WEAK;
- defined: section index != UNDEF;
- visibility: DEFAULT or PROTECTED.

Skip as ordinary misses:

- LOCAL binding;
- UNDEF;
- HIDDEN / INTERNAL.

If the exact name matches but its semantics require unsupported handling, fail explicitly instead of silently selecting a later object:

- XINDEX;
- COMMON;
- TLS;
- GNU IFUNC / other ABI-specific type requiring execution;
- unknown binding/visibility/type that cannot safely be interpreted.

Supported guest value:

- ABS: raw `st_value`;
- normal definition: checked `load_bias + st_value`.

No host pointer is produced.

## Weak Behavior

Runtime dynamic-linker weak semantics are not identical to static link-edit precedence.

For this feature, per-object and graph lookup use first eligible definition semantics: a weak definition found earlier in the deterministic runtime scope is returned and a later global definition is not preferred merely because it is strong.

This matches the simple first-definition runtime behavior used by Android bionic for ordinary unversioned lookup.

Whether an unresolved weak relocation becomes zero is deferred until relocation application, because lookup by name alone does not carry the referencing symbol's binding.

## Versioning

Name-only lookup is unsafe when version metadata changes which same-name definition is eligible.

Before graph lookup searches an object, detect relevant version dynamic tags. If present, return `UnsupportedVersioning` in this first feature.

This is intentionally strict. Version support requires its own accepted contract for:

- versym indexes;
- default vs non-default definitions;
- verdef/verneed traversal;
- hidden version bit;
- versioned weak behavior.

## Graph Scope

Feature 005 explicitly states that object-vector order is not symbol scope.

Build local scope with a queue:

```text
enqueue start
while queue non-empty:
    pop front
    if already visited: continue
    append to scope
    enqueue dependency targets in stored edge order
```

This yields breadth-first order:

```text
A -> B, C
B -> D
C -> E
scope: A, B, C, D, E
```

Cycles/shared objects are visited once.

The design mirrors modern Android's breadth-first local dependency closure where useful, but deliberately omits process-wide global groups, namespaces, preloads, and target-SDK policy.

## Error Model

Expected categories include:

- InvalidOptions
- MissingStringTable
- MissingSymbolTable
- MissingHashTable
- HashHeaderReadFailed
- InvalidSysvHash
- InvalidGnuHash
- HashLimitExceeded
- HashIndexOutOfRange
- UnterminatedGnuChain
- SymbolCountExceeded
- SymbolRangeOverflow
- SymbolReadFailed
- StringReadFailed
- InvalidLookupName
- SymbolNotFound
- UnsupportedBinding
- UnsupportedType
- UnsupportedVisibility
- UnsupportedSectionIndex
- UnsupportedVersioning
- ValueOverflow
- InvalidGraphStart
- ScopeLimitExceeded

Nested linker-string errors should be retained when a candidate name read fails.

## Ownership / Lifetime / Mutation

The symbol layer owns only small result/index metadata and returned strings.

It borrows:

- `GuestMemory`;
- validated linker metadata;
- dependency graph.

It does not retain provider pointers or mutate mappings, guest bytes, object images, or graph edges.

## Documentation Obligations

Because this layer introduces a cross-module public contract, source headers must document:

- caller-selected resource ceilings;
- read-only/mutation guarantees;
- graph BFS scope semantics;
- first-definition weak behavior;
- guest-value vs host-pointer meaning;
- unsupported version/TLS/IFUNC boundaries.

`docs/architecture/elf32-symbol-resolution.md` becomes the durable subsystem contract during T005, with interim semantics captured in this design.

## Test Strategy

### T001 — Hash metadata + bounded symbol index

Synthetic guest memory covers:

- metadata collection/rebasing for `DT_HASH` and `DT_GNU_HASH`;
- duplicate hash tags;
- header address overflow/read failures;
- valid SysV `nchain` count;
- zero/oversized bucket/chain values;
- SysV index-range validation;
- valid GNU-only count;
- GNU zero/power-of-two bloom requirements;
- bucket below symoffset;
- unterminated/oversized GNU chain;
- both-hash count bounds;
- complete symbol-table range validation.

### T002 — Per-object lookup

Synthetic hash/symbol/string tables cover:

- SysV and GNU success;
- collisions;
- exact byte-name match;
- local/undefined/hidden/internal skip;
- global/weak/protected success;
- weak result returned before any later same-object candidate where hash order finds it first;
- ABS vs rebased values;
- candidate string errors;
- TLS/common/XINDEX/IFUNC-or-unknown matching failure;
- empty request rejection;
- no guest mutation.

### T003 — Graph scope / graph lookup

Synthetic `Elf32DependencyGraph` cases cover:

- BFS A -> B,C; B -> D; C -> E;
- duplicate edges;
- cycle A -> B -> A;
- shared dependency;
- object-vector order deliberately different from BFS order;
- first eligible definition wins;
- weak definition before later global wins;
- malformed/unsupported earlier searchable object fails rather than being skipped;
- no-symbol-table object skipped;
- scope-object ceiling.

### T004 — Real ARM32 fixture

Load the pinned NDK fixture through the graph loader, then resolve through GNU hash:

- `fixture_add`;
- `fixture_data`;
- `fixture_bss`.

Require:

- each symbol is found in object 0;
- data symbol resolved value reads `0x12345678`;
- BSS symbol resolved value reads zero;
- function symbol resolves into an executable loaded segment;
- no symbol lookup mutates guest mappings/bytes beyond the existing loaded fixture state.

### T005 — Convergence

Update:

- README current phase;
- architecture symbol-resolution doc;
- metadata/dependency docs if boundaries changed;
- `.agent/STATE.md` and `.agent/NEXT.md`;
- task statuses and exact CI evidence.

Run final exact-head CI.

## Performance Strategy

- Hash-directed lookup only; no full linear name scan.
- Symbol-table range validation is linear once per built index and bounded by `max_symbols`.
- GNU-only count derivation scans buckets plus only the final bucket's chain.
- Graph BFS is linear in bounded objects/edges.
- No index cache is required initially; API/results permit one later if profiling justifies it.

## Alternatives Considered

### Use section headers to obtain `.dynsym` size

Rejected. Runtime-loaded ELF may not retain/use section headers, and the existing architecture is program-header/dynamic-array based.

### Infer symbol count from STRTAB or relocation indexes

Rejected. Neither gives a trustworthy complete dynamic-symbol extent.

### Require SysV `DT_HASH` only

Rejected. The pinned Android NDK fixture uses GNU hash and modern Android objects commonly do so.

### Ignore GNU bloom filter and scan dynsym

Rejected. It would require an independently known symbol count and would discard the format's lookup contract.

### Treat graph object-vector order as search order

Rejected. Feature 005 explicitly preserves that vector as discovery order, not symbol-scope semantics.

### Prefer a later global symbol over an earlier weak symbol

Rejected for this runtime-lookup feature. Android's ordinary runtime lookup returns the first eligible definition in scope; static link-edit weak precedence is not the same operation.

### Silently ignore symbol versions

Rejected. Same-name definitions can differ by version, so name-only success could be wrong.

### Implement relocations at the same time

Rejected. Hash/symbol lookup and graph scope are independently testable correctness boundaries needed before relocation writes.

## Assumptions

- The first supported objects are ARM ELF32 little-endian ET_EXEC/ET_DYN already accepted by the loader.
- GNU hash bloom words are 32-bit for ELF32.
- The graph is immutable while lookup runs.
- No concurrent guest mapping mutation occurs during the read-only lookup call.
- The pinned fixture continues exporting its three default-visible symbols unless its source/build recipe changes.
