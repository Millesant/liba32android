# Next Work

Repository integration is on `bleeding`. The current control-plane round is
pinned to
`millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

Features 011 through 024 are DONE. The latest behavior-changing result is
feature 024 at `d4e7b480e28d13e8edc5dd1ccf28abbc3072a7a1`, which passed
Linux A32 smoke and both required Android checks.

`project-cleanup-v9` is DONE at
`567ab4931d3d0af69edb7680b0a266be7672df64`; Linux A32 smoke and both required
Android checks passed. Canonical state/current architecture docs now distinguish
implemented downstream behavior, layer-local non-goals, genuine deferred gaps,
and historical numbered-spec snapshots.

## Candidate runtime directions

The supplied VLC ARMv7/FMOD evidence shows the observed relocation set is
already inside the implemented REL/JUMP_SLOT subset. Provider composition,
exact-name catalogs, INIT_ARRAY planning/execution, resumable SVC state, and
bounded host-service dispatch are implemented.

Current bounded candidates are:

- implement concrete ARM32 compatibility shim stubs plus a small host-service
  registry for the highest-priority VLC/FMOD platform imports;
- add Android namespace/search/path policy above the requester-aware provider
  chain/catalog boundary;
- add persisted lifecycle state plus FINI/destructor/unload ordering;
- add broader relocation/TLS/IFUNC support only when a concrete target requires
  it;
- obtain end-to-end real ARM32 runtime execution evidence on Android when the
  required environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
