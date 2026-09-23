# ELF32 ARM REL relocation application

Status: complete; feature 007 final exact-head gate PASSed

## Boundary

`elf32_relocation` is the first guest-memory-mutating dynamic-linker layer above the loaded `Elf32DependencyGraph`, validated linker metadata, bounded linker-string access, and feature-006 graph-local symbol lookup.

It consumes only logical 32-bit guest state:

- `memory::GuestMemory`;
- one immutable loaded dependency graph for the duration of the call;
- one relocating object index;
- the object's validated main `DT_REL` descriptor and load bias;
- bounded feature-006 symbol/hash/string/scope options.

It does not acquire dependencies, choose guest bases, implement pathname/namespace/global-group policy, process PLT/JMPREL, execute guest code, or convert guest addresses into host pointers.

## Plan before mutation

The layer treats linker metadata as a validated descriptor, not as a host structure. Each 8-byte `Elf32_Rel` entry is decoded explicitly as little-endian `r_offset` and `r_info`; symbol index is `r_info >> 8` and relocation type is `r_info & 0xff`.

Planning is bounded by caller-selected `max_relocations`. For every supported write-producing entry it:

1. computes `P = load_bias + r_offset` with checked 32-bit guest arithmetic;
2. requires 4-byte alignment;
3. reads the original 32-bit target word through `GuestMemory`;
4. retains that word as the REL addend where the formula uses `A` and as rollback state;
5. rejects a duplicate write-producing target before mutation.

`R_ARM_NONE` remains represented in the plan but performs no target read, lookup, or write. Unsupported relocation types fail during planning before mutation.

## Reference resolution

`R_ARM_ABS32` and `R_ARM_GLOB_DAT` use the relocating object's dynamic-symbol index and exact string-table name, then feature-006 graph-local breadth-first lookup beginning at that object.

The relocation policy accepts ordinary `STB_GLOBAL` / `STB_WEAK`, `STV_DEFAULT`, NOTYPE/OBJECT/FUNC references. It explicitly rejects protected requester semantics, hidden/internal/local references, TLS, IFUNC, COMMON, XINDEX, and versioned objects rather than approximating semantics that the runtime does not yet model.

A graph miss is an error for a strong reference. For a weak reference it becomes `S = 0` for the supported absolute relocation forms. That weak-to-zero rule belongs to relocation policy; the symbol-lookup layer remains a read-only name-resolution primitive.

## Supported formulas

The first bounded ARM set is:

- `R_ARM_NONE` (0): no write;
- `R_ARM_RELATIVE` (23): requires symbol index zero and stores `B + A` modulo 2^32, where `B` is the relocating object's load bias;
- `R_ARM_GLOB_DAT` (21): stores `S` exactly;
- `R_ARM_ABS32` (2): stores `S + A` modulo 2^32.

For ARM `R_ARM_GLOB_DAT`, the runtime intentionally follows current Android bionic behavior and ignores the in-place REL addend. The synthetic regression uses a nonzero original word so this Android-specific choice cannot silently drift back to the generic AAELF32 formula.

Resolved symbol values are logical ELF32 guest values and are used as returned by feature 006. No host-pointer or instruction-decoding semantics are introduced here.

## Transaction and rollback contract

Application has two phases. First, the complete table is decoded, all required target reads and symbol/string/hash lookups succeed, `R_ARM_RELATIVE` symbol-index constraints are checked, and every final 32-bit result is computed. No relocation write occurs before that phase is complete.

Second, writes are issued through `GuestMemory::write` in REL table order. If a later write fails, every earlier successful relocation-owned write is restored in reverse order from the captured original words.

A successful rollback returns the primary target-write failure with no published successful application writes. If restoration itself fails, the result is `RollbackFailed`, preserves the primary `TargetWriteFailed`, and identifies the rollback-failing relocation when available. Only that explicit rollback-failure case may leave guest state partially relocated.

The layer never changes guest page permissions to make a write succeed. Text-relocation compatibility and permission transitions remain separate policy.

## Address, ownership, and compatibility invariants

- Guest VAs and relocation results are logical 32-bit values, never host pointers.
- All guest bytes are accessed through `GuestMemory`.
- The graph is borrowed and must remain immutable for one plan/apply call.
- Plans own only small decoded metadata plus original/final 32-bit words.
- Existing loader, metadata, dependency, and symbol APIs remain source-compatible apart from additive symbol-decoding support already introduced for feature 007.
- Failure before the commit phase leaves relocation targets unchanged.

## Validation evidence

Feature 007 was built in four executable slices before closeout:

- T001 read-only REL planning: exact-head CI #210 / run `35850236188` at `2b4e9185bac43fe9bb46ddf8c7da9b73e0146837`;
- T002 relocation-reference resolution: CI #212 / run `35889244367` at `650d7b262540360ba2395a802ba7d7766566d544`;
- T003 transactional application/rollback: CI #214 / run `35890660951` at `41a93348c29fb884befba5ba8bad51ecf0d49665`;
- T004 pinned real ARM32 application: CI #216 / run `35891830738` at `5d74af22c16a7bc99eee7038dfb9f137b22807c2`.

T003 synthetic coverage locks NONE no-write behavior, RELATIVE `B+A`, bionic-compatible nonzero-addend GLOB_DAT `S`, ABS32 `S+A`, all-semantic-checks-before-write, reverse rollback after a later write failure, and explicit rollback-failure reporting.

The pinned NDK r27d / API 26 ARMv7 fixture contains exactly two main `.rel.dyn` `R_ARM_GLOB_DAT` entries at linked offsets `0x82cc` / `0x82d0` for `fixture_bss` / `fixture_data`. T004 resolves both through feature 006, applies them, verifies each target equals the resolved guest value, preserves initialized data/BSS, keeps dependency-provider calls at zero, and verifies mapping permissions and all non-target segment bytes remain unchanged.

The T005 R1-R15 / AC1-AC15 convergence gate PASSed exact-head CI #218 / run `35918899544` at `8efe792cfa58a3f34e02dfe0c8bb01fbc3949766`. Linux passed 45/45 CTest including all four relocation plan/apply synthetic and real-fixture tests; Android x86_64 address-space probe and Android arm64-v8a cross-build also passed. No blocking semantic gap remains in the bounded feature-007 contract.

## Deliberate limits

Feature 007 does not implement:

- `DT_JMPREL`, PLT/GOT lazy binding, or `R_ARM_JUMP_SLOT`;
- REL32, COPY, instruction relocations, RELA, RELR, Android packed relocations, or APS2;
- symbol-version matching or requester-specific protected/`DT_SYMBOLIC` self-binding;
- Android namespaces, preloads, global groups, or process-wide interposition policy;
- TLS relocations/addressing or GNU IFUNC execution;
- RELRO;
- text-relocation permission broadening;
- constructors/destructors, `dlopen`, `dlsym`, unload, or guest execution.

Those remain separate contracts rather than implicit compatibility behavior.
