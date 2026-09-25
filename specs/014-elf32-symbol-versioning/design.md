# Design — ELF32 symbol versioning

Status: ACTIVE — implementation prepared; exact-head validation NOT RUN

## Metadata

The linker-metadata layer now retains three bounded guest descriptors:

- DT_VERSYM: guest address of 16-bit symbol-version entries;
- DT_VERDEF + DT_VERDEFNUM: guest address and top-level definition count;
- DT_VERNEED + DT_VERNEEDNUM: guest address and top-level requirement count.

VERDEF and VERNEED pointer/count pairs are all-or-nothing. Version names continue to use the already-validated dynamic string table.

## Request-side resolution

A relocation supplies its requester dynamic-symbol index. The matching VERSYM entry is read from guest memory and masked with `0x7fff`.

Indices 0/1 carry no explicit version. Higher indices are resolved by bounded VERNEED/Vernaux traversal. The VERNEED `vn_file` name must identify a direct dependency by SONAME, matching Android/bionic's validation posture. Requester VERDEF records are processed after VERNEED so a same-index local definition can supersede a requirement.

## Provider-side matching

If a provider has no VERSYM table, its otherwise-eligible symbol can satisfy the request.

For an unversioned request, a provider candidate is eligible only when the VERSYM hidden bit `0x8000` is clear.

For an explicit request, VERDEF is searched by the request's stored ELF hash and exact version name. A match selects `vd_ndx & 0x7fff`. If no VERDEF matches, the expected provider version is global index 1. The candidate's hidden bit is ignored for explicit matching.

## Scope

Version filtering is inserted inside the existing object lookup, so SysV/GNU hash traversal and graph-local BFS ordering remain unchanged. Relocation resolution now derives the requester version and calls the same graph search. No new mutation occurs in the version layer.

## Real fixture

A pinned-NDK freestanding provider exports `fixture_versioned_import@@LIBC` through a linker version script. A consumer linked against that DSO carries a VERNEED `LIBC` reference and one JUMP_SLOT relocation. CI regenerates both DSOs twice, compares bytes, inspects version/relocation metadata with readelf, and applies the relocation through the real loader graph.
