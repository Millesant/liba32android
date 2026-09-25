# ELF32 symbol versioning

Status: feature 014 implementation prepared; exact-head validation NOT RUN

Symbol versioning is a read-only filter layered onto the existing dynamic-symbol lookup and relocation-resolution path.

## Accepted implementation boundary

The implementation understands DT_VERSYM, DT_VERDEF/DT_VERDEFNUM, and DT_VERNEED/DT_VERNEEDNUM. All addresses remain logical 32-bit guest virtual addresses and all table reads go through `GuestMemory`.

Traversal is bounded by `Elf32SymbolLookupOptions::max_version_records` plus the existing string-size ceiling. Version metadata does not add a new mutation path.

The version layer preserves the existing graph-local breadth-first lookup order. It does not approximate Android namespace/global-group policy or DT_SYMBOLIC.

## Matching rules

Requester VERSYM indices 0 and 1 mean no explicit version. Higher indices are resolved to a version name/hash from VERNEED or requester VERDEF metadata.

Unversioned lookup skips hidden provider definitions. An explicit request matches a provider VERDEF name/hash when present; without a matching provider VERDEF the candidate must use global version index 1.

A VERNEED library name is accepted only when it corresponds to a direct dependency SONAME.

## Validation target

Feature 014 includes a generated ARMv7 pair in which the provider exports a symbol under version `LIBC` and the consumer records a versioned JUMP_SLOT reference. This mirrors the version shape observed in the supplied VLC ARMv7 libraries without storing third-party binaries in the repository.
