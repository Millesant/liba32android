# ELF32 symbol versioning

Status: feature 014 DONE — exact-head implementation CI PASSed at `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26`

Symbol versioning is a read-only filter layered onto the existing dynamic-symbol lookup and relocation-resolution path.

## Accepted implementation boundary

The implementation understands DT_VERSYM, DT_VERDEF/DT_VERDEFNUM, and DT_VERNEED/DT_VERNEEDNUM. All addresses remain logical 32-bit guest virtual addresses and all table reads go through `GuestMemory`.

Traversal is bounded by `Elf32SymbolLookupOptions::max_version_records` plus the existing string-size ceiling. Version metadata does not add a new mutation path.

The version layer preserves the existing graph-local breadth-first lookup order. It does not approximate Android namespace/global-group policy or DT_SYMBOLIC.

## Matching rules

Requester VERSYM indices 0 and 1 mean no explicit version. Higher indices resolve to a version name/hash from VERNEED or requester VERDEF metadata.

Unversioned lookup skips hidden provider definitions. An explicit request matches a provider VERDEF name/hash when present; without a matching provider VERDEF the candidate must use global version index 1.

A VERNEED library name is accepted only when it corresponds to a direct dependency SONAME.

## Exact-head evidence

At `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26`, Linux A32 smoke check `108040133539`, Android arm64-v8a cross-build check `108040133332`, and Android x86_64 address-space probe check `108040133467` all PASSed. Linux validates the synthetic version edge cases and a reproducible generated ARM32 `LIBC`-versioned JUMP_SLOT consumer/provider pair.

DT_SYMBOLIC/self-first ordering, process/global groups, namespaces, compatibility shims, constructors, TLS/IFUNC, and Android-device execution remain separate work.
