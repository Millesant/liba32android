# ELF32 symbol versioning

Status: feature 014 DONE — exact-head implementation CI PASSed at `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26`

Symbol versioning is a read-only filter layered onto the existing dynamic-symbol lookup and relocation-resolution path.

## Accepted implementation boundary

The implementation understands DT_VERSYM, DT_VERDEF/DT_VERDEFNUM, and DT_VERNEED/DT_VERNEEDNUM. All addresses remain logical 32-bit guest virtual addresses and all table reads go through `GuestMemory`.

Traversal is bounded by `Elf32SymbolLookupOptions::max_version_records` plus the existing string-size ceiling. Version metadata does not add a new mutation path.

The version filter does not choose candidate-object order. Feature 015 composes the same matching rules with relocation/reference scope ordering: ordinary requesters may search a caller-provided in-graph global list before their local breadth-first closure, while DT_SYMBOLIC/DF_SYMBOLIC requesters search themselves first. Android namespace/global-group construction and process-wide link-map lifetime remain outside this layer.

## Matching rules

Requester VERSYM indices 0 and 1 mean no explicit version. Higher indices resolve to a version name/hash from VERNEED or requester VERDEF metadata.

Unversioned lookup skips hidden provider definitions. An explicit request matches a provider VERDEF name/hash when present; without a matching provider VERDEF the candidate must use global version index 1.

A VERNEED library name is accepted only when it corresponds to a direct dependency SONAME.

## Exact-head evidence

At `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26`, Linux A32 smoke check `108040133539`, Android arm64-v8a cross-build check `108040133332`, and Android x86_64 address-space probe check `108040133467` all PASSed. Linux validates the synthetic version edge cases and a reproducible generated ARM32 `LIBC`-versioned JUMP_SLOT consumer/provider pair.

Process-wide/global-group construction, namespaces, compatibility shims, constructors, TLS/IFUNC, and Android-device execution remain separate work. Completed feature 015 implements DT_SYMBOLIC/DF_SYMBOLIC requester-first ordering and reuses this version filter.
