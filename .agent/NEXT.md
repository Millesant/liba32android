# Next Work

Repository integration is on `bleeding`. The current control-plane round is
pinned to
`millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

Features 011 through 025 are DONE. The latest behavior-changing result is
feature 025 at `c2fb94e450619378bb0b77880fb0454e53ce38b3`, which passed
Linux A32 smoke and both required Android checks.

`project-cleanup-v9` is DONE at
`567ab4931d3d0af69edb7680b0a266be7672df64`; Linux A32 smoke and both required
Android checks passed. Canonical state/current architecture docs distinguish
implemented downstream behavior, layer-local non-goals, genuine deferred gaps,
and historical numbered-spec snapshots.

## Candidate runtime directions

The supplied VLC ARMv7/FMOD evidence shows the observed relocation set is
already inside the implemented REL/JUMP_SLOT subset. Provider composition,
exact-name catalogs, INIT_ARRAY planning/execution, resumable SVC state,
bounded host-service dispatch, and exact service-number registry composition are
implemented.

A fresh bounded static scan of the supplied artifacts is recorded in
`docs/research/evidence/android-log-imports-2026-09-26.md`. It shows both the
supplied ARM32 `libfmod.so` and VLC's ARMv7 `libvlc.so` import
`__android_log_write`, making that symbol the narrowest shared concrete
platform-service target observed so far.

Current bounded candidates are:

- verify the authoritative Android API/ABI contract for
  `__android_log_write`, then implement the smallest bounded host-service
  adapter plus guest compatibility-shim path for that one symbol without
  pulling varargs `__android_log_print`/`__android_log_vprint` into the same
  change;
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
