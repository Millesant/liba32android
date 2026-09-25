# Requirements — ELF32 symbol versioning

Status: ACTIVE — implementation prepared; exact-head validation NOT RUN

## Goal

Allow supported ARM32 relocation references in versioned Android ELF DSOs to resolve through the existing dependency graph without weakening bounds, mutation discipline, or guest-address isolation.

## Requirements

- Retain and validate DT_VERSYM, DT_VERDEF/DT_VERDEFNUM, and DT_VERNEED/DT_VERNEEDNUM as guest-only descriptors.
- Preserve duplicate-singleton, completeness, rebasing, range, and readable-memory validation.
- Add an explicit caller ceiling for version-record traversal.
- Decode Elf32_Verneed/Vernaux and Elf32_Verdef/Verdaux byte-explicitly through GuestMemory.
- Treat VERSYM indices 0 and 1 as no explicit version requirement.
- Resolve higher requester indices through VERNEED and requester VERDEF metadata.
- Validate each VERNEED target SONAME against the requester's direct dependency edges.
- For unversioned lookup, ignore hidden provider definitions.
- For an explicit version request, match provider VERDEF by ELF hash plus exact version name; if no matching VERDEF exists, compare against global version index 1.
- Preserve current symbol eligibility, hash-index, graph BFS, strong/weak, relocation transaction, and rollback semantics.
- Keep malformed/oversized version metadata read-only and fail explicitly.
- Add reproducible real ARM32 consumer/provider evidence with a version named `LIBC`.

## Non-goals

DT_SYMBOLIC/self-first relocation ordering, Android global groups/namespaces, process-wide interposition, dlvsym, TLS/IFUNC, constructors, lazy PLT binding, and compatibility-library implementations.
