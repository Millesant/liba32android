# Requirements — ELF32 symbol versioning

Status: DONE — exact-head implementation CI PASSed at `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26`

## Goal

Allow supported ARM32 relocation references in versioned Android ELF DSOs to resolve through the existing dependency graph without weakening bounds, mutation discipline, or guest-address isolation.

## Accepted requirements

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
- Maintain a reproducible real ARM32 consumer/provider fixture with a version named `LIBC`.

## Validation

At `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26`, Linux A32 smoke check `108040133539`, Android arm64-v8a cross-build check `108040133332`, and Android x86_64 address-space probe check `108040133467` PASSed. Linux includes the generated versioned fixture and explicit version-aware JUMP_SLOT application.
