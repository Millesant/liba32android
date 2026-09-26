# ELF32 symbol resolution

Status: complete; feature 006 final exact-head gate PASSed

## Boundary

`elf32_symbol_lookup` is the first bounded dynamic-symbol semantics layer above validated linker metadata/string access and the loaded dependency graph.

It consumes:

- `memory::GuestMemory` for all hash, symbol, and string reads;
- validated `Elf32LinkerMetadata` including optional `DT_HASH` / `DT_GNU_HASH` descriptors and the symbol-version-presence marker;
- a loader-provided logical 32-bit load bias for per-object lookup;
- `Elf32DependencyGraph` edges for graph-local lookup scope;
- explicit caller-selected resource ceilings.

It returns small host-owned index/result metadata and logical 32-bit guest values only. It never returns host pointers, mutates guest bytes/mappings, changes graph edges, invokes the dependency provider, applies relocations, or executes guest code.

```text
ELF32 image
   |
   v
loader -> dynamic -> linker metadata -> linker strings
                              |
                              +--> dependency loader -> loaded object graph
                              |
                              v
                    elf32_symbol_lookup
                      |              |
                      |              +--> graph-local BFS scope
                      v
              SysV/GNU hash index
                      |
                      v
             exact symbol definition
                      |
                      v
          logical 32-bit guest value
                      |
                      v
                 elf32_relocation
```

## Hash metadata and symbol extent

The metadata layer rebases `DT_HASH` and `DT_GNU_HASH` exactly once and validates only their fixed headers. Variable arrays belong here because they require explicit resource ceilings.

For SysV hash:

- `nbucket` and `nchain` must be non-zero and within caller ceilings;
- all bucket/chain indexes are bounded by `nchain`;
- the exact dynamic-symbol count is `nchain`;
- traversal is additionally bounded by that count.

For ELF32 GNU hash:

- bucket count and bloom-word count must be non-zero;
- bloom-word count must be a power of two;
- bucket/bloom counts must fit caller ceilings;
- non-zero buckets must begin at or after `symoffset`;
- derived array addresses use checked 32-bit guest arithmetic;
- chain traversal is bounded by `max_symbols` and must terminate via the low-bit marker.

When GNU hash is the only supported format, symbol count is derived from the maximum non-zero bucket and its terminating chain; all-zero buckets yield `symoffset`. When both formats exist, SysV `nchain` is the authoritative full extent and GNU indexes must remain inside it.

After count derivation, the complete `Elf32_Sym` range is validated read-only before success. Section headers are not used.

## Per-object lookup

`lookup_elf32_symbol` accepts an exact non-empty byte name.

- GNU hash is preferred when both formats are present; a GNU miss is not hidden by SysV fallback.
- Candidate names are read through the bounded linker-string reader and compared exactly.
- `STB_GLOBAL` and `STB_WEAK` definitions with `STV_DEFAULT` or `STV_PROTECTED` visibility are eligible.
- `STB_LOCAL`, `SHN_UNDEF`, `STV_HIDDEN`, and `STV_INTERNAL` candidates are misses.
- `SHN_ABS` returns raw `st_value`.
- Ordinary supported definitions return checked `load_bias + st_value`.
- The first eligible definition found by the selected hash traversal wins, including a weak definition.

Matching forms that require semantics not implemented here still fail explicitly rather than being approximated: TLS, COMMON, XINDEX, GNU IFUNC or unsupported type/binding/visibility forms. Symbol-version metadata is now interpreted through the bounded feature-014 layer: unversioned lookups skip hidden definitions, while relocation/reference lookups derive explicit version requirements from requester DT_VERSYM and VERNEED/VERDEF records.

The result includes symbol index, raw decoded ELF32 symbol metadata, exact name, and the resolved logical guest value.

## Graph-local scope

`lookup_elf32_graph_symbol` does not use graph object-vector discovery order as symbol scope.

Starting from the caller-selected object, it builds a deterministic breadth-first dependency closure:

1. visit the start object;
2. enqueue its dependency targets in stored edge order;
3. continue breadth-first, preserving each visited object's edge order;
4. visit each object index at most once.

Cycles and shared dependencies therefore terminate naturally. Repeated edges remain ordering inputs but do not cause repeated object lookup.

Objects without a symbol table are skipped. A malformed/versioned searchable object encountered before a definition fails the lookup instead of being silently skipped. The first eligible definition in BFS scope wins, including an earlier weak definition.

Plain `lookup_elf32_graph_symbol` remains intentionally local to one loaded dependency graph and ignores any relocation/reference global-scope option.

## Relocation/reference scope policy

`lookup_elf32_graph_symbol_for_reference` adds one bounded ordering layer without creating a process-wide linker. Callers may provide an ordered `global_scope_objects` span containing object indices from the same already-loaded graph.

- ordinary requesters search the explicit global span first, then the requester's deterministic graph-local BFS closure;
- requesters whose validated metadata carries `DT_SYMBOLIC` or `DF_SYMBOLIC` search themselves first, then the explicit global span, then the remaining local closure;
- the searched-object set is deduplicated, so an object appearing in more than one phase is evaluated once;
- every unique searched object consumes the same `max_scope_objects` budget;
- an out-of-range explicit global index fails as `InvalidGlobalScopeObject`;
- version filtering is applied identically to self, global, and local candidates.

Global candidates are lookup entries, not implicit dependency roots: their dependency edges are not traversed merely because the object appears in the global span.

## Downstream relocation policy

`elf32_relocation` already resolves both main REL and eager PLT references through `lookup_elf32_graph_symbol_for_reference`, so the caller-provided global span and requester symbolic flag flow through that existing reference path without adding a second relocation policy. Returned logical guest symbol values are preserved, protected requester semantics and TLS/IFUNC/common/XINDEX forms remain explicit failures, and an unresolved weak reference maps to `S = 0` only in the relocation context.

## Resource and failure model

Every operation uses caller-selected finite ceilings:

- maximum symbols;
- maximum hash buckets;
- maximum GNU bloom words;
- maximum graph-scope objects;
- maximum symbol-name payload bytes.

Address/count/size arithmetic is checked, hash walks are bounded, and no malformed table can cause unbounded recursion or traversal.

The public result types distinguish index construction failures, object lookup failures, graph-scope failures, nested string errors, and the failing graph object where applicable. Failure never returns a successful partial symbol result.

## Mutation and address invariants

The entire symbol layer is read-only.

- Hash, symbol, and string bytes are read only through `GuestMemory`.
- Guest mappings, permissions, bytes, dependency edges, and provider state are unchanged on success and failure.
- Returned addresses/values are logical 32-bit guest values, never host pointers.
- Relocation writes are implemented by downstream `elf32_relocation`; this symbol layer remains read-only.

The real-fixture integration snapshots loaded segment bytes, mappings, and permissions before lookup and verifies they are unchanged afterward.

## Validation

Feature 006 was advanced in exact-head slices:

- T001 hash metadata/indexing: CI #203 / run `35758444356` PASS at `45cd3e5a322263f53dc02277a9d0e801849515db`.
- T002 exact-name per-object lookup: CI #204 / run `35759553586` PASS at `5f21c8ed48f458f7f3d909fff39523d9ebf9b7e0`.
- T003 graph-local BFS lookup: CI #205 / run `35760283793` PASS at `f3997d037f7f5a29b1666dd9a6f5a566b249a2cc`.
- T004 pinned real ARM32 GNU-hash integration: CI #206 / run `35837480789` PASS at `2fdba16a13e34370483701345de2605df06811e6`.

T004 Linux validation passed 41/41 CTest including `elf32_symbol_index` and `elf32_real_symbol_lookup`; Android x86_64 address-space probe and Android arm64-v8a cross-build jobs also passed. The real fixture resolves `fixture_add`, `fixture_data`, and `fixture_bss` from object 0, reads `fixture_data == 0x12345678`, reads zero BSS, verifies the function value lies in an executable segment, and verifies lookup does not mutate loaded state.

T005 documentation/state/spec convergence and the final feature-head gate PASSed exact-head CI #207 / run `35847914558` at `ad022c2cc569c3175ad1cef0140f964817f5a820`. Linux passed 41/41 CTest including `elf32_real_symbol_lookup`; Android x86_64 address-space probe and Android arm64-v8a cross-build also passed. Feature 006 is complete.

## Deliberate layer-local limits

These are symbol-layer ownership boundaries, not a repository-wide list of
missing functionality.

This layer does not itself:

- write relocations; bounded main REL and eager PLT/JUMP_SLOT writes are implemented in `elf32_relocation`, while lazy binding/DT_PLTGOT resolver state remains deferred;
- define Android namespace/preload/search-path/RTLD policy; persistent link-map/global-group construction and generic requester-aware provider chaining/catalogs exist downstream, while Android policy remains deferred;
- implement protected-reference self-binding beyond the current accepted relocation policy;
- calculate TLS addresses/relocations or execute GNU IFUNC;
- seal RELRO or run lifecycle code; RELRO plus bounded INIT_ARRAY planning/execution are implemented downstream, while destructor/unload lifecycle and `dlopen`/`dlsym` remain deferred;
- execute guest code; host fixture and lifecycle execution occur in downstream integration/runtime layers.

Those limits are explicit compatibility boundaries, not silent fallbacks.


## Feature 014 — bounded symbol-version matching

Feature 014 retains validated DT_VERSYM, VERNEED, and VERDEF descriptors and adds a caller-bounded version-record walk. VERNEED library names must identify direct dependency SONAMEs. Request indices 0/1 remain unversioned; explicit requests carry the recorded ELF hash and exact version name. Provider matching skips hidden definitions only for unversioned requests, and explicit requests select a matching VERDEF index or fall back to global index 1.

Exact-head implementation CI at `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26` passed Linux A32 smoke check `108040133539`, Android arm64-v8a cross-build check `108040133332`, and Android x86_64 address-space probe check `108040133467`. The Linux gate includes a generated freestanding ARM32 consumer/provider pair whose JUMP_SLOT import is versioned `LIBC`.


## Feature 015 — bounded requester/global scope ordering

Feature 015 retains `DT_SYMBOLIC` and the `DF_SYMBOLIC` bit of `DT_FLAGS` as validated linker metadata, adds caller-owned ordered global-scope candidates to relocation/reference lookup, and preserves plain graph-local lookup as-is. Synthetic metadata, symbol-lookup, and relocation tests cover complete explicit-global-index preflight, global-before-local ordering, requester-first symbolic binding, shared scope ceilings, and main-REL plus PLT inheritance. Exact-head GitHub Actions CI run `36193971238` PASSed at result revision `2ed5157504dc9d7affac2290b1a19535b28913f9` across Linux A32 smoke and both required Android lanes. Feature 015 is complete.


## Feature 016 — persistent link map/global group

Feature 016 adds the producer side of the feature-015 global-scope contract.
A caller-owned `Elf32LinkMap` preserves stable object indexes and mappings
across root appends, derives global membership from caller-designated global
roots plus validated `DF_1_GLOBAL`, and maintains the resulting indexes in
accumulated object-discovery order. The span returned by
`Elf32LinkMap::global_scope()` is consumed directly by reference lookup; no
new symbol-matching rule is introduced.

Exact-head GitHub Actions CI run `36201652255` (#299) PASSed at
`0c374ff84990ee3d64c06a1037f846d90054e5e4` across Linux A32 smoke and both
required Android lanes.
