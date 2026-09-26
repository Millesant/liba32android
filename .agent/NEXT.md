# Next Work

Repository integration is on `bleeding`. The current control-plane round is
pinned to
`millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

Features 011 through 024 are DONE. The latest accepted behavior-changing result
is feature 024 at `d4e7b480e28d13e8edc5dd1ccf28abbc3072a7a1`, which passed
Linux A32 smoke and both required Android checks.

`025-a32-host-service-registry` is IMPLEMENTED with source, runtime regression,
CMake registration, current runtime-contract delta, architecture documentation,
and durable change records prepared. Its exact-head Linux A32 smoke and both
required Android checks are NOT RUN; closing those checks is the immediate next
step before feature 025 may become accepted state.

`project-cleanup-v9` is DONE at
`567ab4931d3d0af69edb7680b0a266be7672df64`; Linux A32 smoke and both required
Android checks passed. Canonical state/current architecture docs now distinguish
implemented downstream behavior, layer-local non-goals, genuine deferred gaps,
and historical numbered-spec snapshots.

## Candidate runtime directions

The supplied VLC ARMv7/FMOD evidence shows the observed relocation set is
already inside the implemented REL/JUMP_SLOT subset. Provider composition,
exact-name catalogs, INIT_ARRAY planning/execution, resumable SVC state, and
bounded host-service dispatch are accepted. Feature 025 adds the generic exact
service-number composition seam once exact-head validation closes.

After feature 025 acceptance, current bounded candidates are:

- implement the first concrete ARM32 compatibility shim/service pair for a
  highest-priority observed VLC/FMOD platform import, keeping register-only ABI
  handling separate from later stack/varargs marshalling;
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
