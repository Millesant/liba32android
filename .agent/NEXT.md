# Next Work

Repository integration is on `bleeding`. The current control-plane round is
pinned to
`millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

Features 011 through 025 are DONE. The latest accepted behavior-changing result
is feature 025 at `c2fb94e450619378bb0b77880fb0454e53ce38b3`, which passed
Linux A32 smoke and both required Android checks.

`026-a32-android-log-write-service` is IMPLEMENTED with the compatibility
adapter, bounded GuestMemory string handling, sink boundary, compatibility
regression/CMake registration, API/AAPCS evidence, current compatibility spec,
architecture docs, and durable change records prepared. Its exact-head Linux
A32 smoke and both required Android checks are NOT RUN; closing those checks is
the immediate next step before feature 026 may become accepted state.

## Candidate runtime directions

The supplied ARM32 FMOD library and VLC ARMv7 `libvlc.so` both import
`__android_log_write`. Feature 026 models that three-register call without
pulling `__android_log_print`/`__android_log_vprint` varargs into the same
change.

After feature 026 acceptance, current bounded candidates are:

- build the smallest guest ELF `liblog.so` compatibility-shim/provider path
  that exports `__android_log_write` and traps through the now-verified
  service bridge, without adding print/vprint yet;
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
