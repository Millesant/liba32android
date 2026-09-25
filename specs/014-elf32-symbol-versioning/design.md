# Design — ELF32 symbol versioning

Status: DONE — exact-head implementation CI PASSed at `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26`

## Metadata

The linker-metadata layer retains guest descriptors for DT_VERSYM, DT_VERDEF/DT_VERDEFNUM, and DT_VERNEED/DT_VERNEEDNUM. Pointer/count pairs are validated before downstream interpretation.

## Request-side resolution

A relocation supplies its requester dynamic-symbol index. The VERSYM entry is read from GuestMemory and masked with `0x7fff`. Indices 0/1 carry no explicit version. Higher indices are resolved through bounded VERNEED/Vernaux records; requester VERDEF is processed afterward to match bionic's tracker ordering. VERNEED `vn_file` must identify a direct dependency SONAME.

## Provider-side matching

A provider without VERSYM remains eligible. For unversioned requests, hidden provider versions are skipped. For explicit requests, provider VERDEF is searched by recorded ELF hash and exact name; absent a matching definition, global version index 1 is required.

## Integration

Version filtering is inside object lookup, preserving existing SysV/GNU hash traversal and graph-local BFS. Relocation resolution passes the requester symbol index to the version-aware graph entry point. No new guest-memory mutation is introduced.

The generated ARMv7 provider exports `fixture_versioned_import@@LIBC`; the consumer carries a `LIBC` VERNEED and JUMP_SLOT reference. CI rebuilds both reproducibly and applies the reference through the real dependency graph.
