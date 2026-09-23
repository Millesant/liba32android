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

Matching forms that require semantics not implemented here fail explicitly rather than being approximated: TLS, COMMON, XINDEX, GNU IFUNC or other unsupported type/binding/visibility forms. If the object declares symbol-version metadata, name-only lookup returns `UnsupportedVersioning`.

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

This scope is intentionally local to one loaded dependency graph. Android global groups, namespaces, preloads, requester-sensitive policy, and process-wide link-map lifetime remain outside this feature.

## Downstream relocation policy

Feature 007 consumes this layer without widening its lookup contract. `elf32_relocation` starts graph-local BFS lookup at the relocating object, preserves returned logical guest symbol values, rejects protected requester semantics and versioned/TLS/IFUNC/common/XINDEX reference forms explicitly, and maps an unresolved weak reference to `S = 0` only in the relocation context. Those mutation/reference-policy decisions do not belong in this read-only symbol layer.

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

## Deliberate limits

This feature does not implement:

- relocation writes are outside this layer and live in `elf32_relocation`; PLT/GOT/JMPREL and lazy binding remain unimplemented;
- symbol version matching;
- Android/global-group/namespace/preload/process-wide interposition policy;
- requester-specific `DT_SYMBOLIC` / protected self-binding relocation semantics;
- TLS address calculation or TLS relocations;
- GNU IFUNC execution;
- RELRO, constructors/destructors, `dlopen`, `dlsym`, or unload;
- guest execution.

Those limits are explicit compatibility boundaries, not silent fallbacks.
