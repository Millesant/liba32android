# Next Work

Repository integration is on `bleeding`. The current control-plane round is
pinned to
`millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

Features 011 through 026 are DONE. The latest behavior-changing result is
feature 026 at `59fa3eba1d53c6679ef6086cd209198ca7ecb4ac`, which passed
Linux A32 smoke and both required Android checks.

The supplied ARM32 FMOD library and VLC ARMv7 `libvlc.so` both import
`__android_log_write`. Feature 026 now provides the verified bounded
AAPCS32 host-service side of that call while deliberately leaving guest ELF
symbol provisioning separate.

## Candidate runtime directions

Current bounded candidates are:

- build the smallest guest ELF `liblog.so` compatibility-shim/provider path
  that exports `__android_log_write` and traps through feature 026, using the
  existing exact-name provider/dependency/linker machinery and keeping
  `__android_log_print`/`__android_log_vprint` varargs out of scope;
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
