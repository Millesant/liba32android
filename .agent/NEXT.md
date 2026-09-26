# Next Work

Repository integration is on `bleeding`. The current control-plane round is
pinned to
`millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

Features 011 through 026 are DONE. The latest accepted behavior-changing result
is feature 026 at `59fa3eba1d53c6679ef6086cd209198ca7ecb4ac`.

`027-a32-liblog-write-shim-provider` is IMPLEMENTED. It adds the reproducible
ARM32 `liblog.so` write shim, shared private SVC definition, exact-name catalog
helper, real consumer/shim integration regression, CI fixture generation, and
current specs/docs. Exact-head Linux A32 smoke plus both Android checks are NOT
RUN and are the immediate acceptance gate.

After feature 027 acceptance, the nearest bounded directions are:

- decide whether to package/install the generated `liblog.so` shim through a
  concrete platform catalog or first add Android namespace/search policy around
  the existing requester-aware provider chain;
- add `__android_log_print`/`__android_log_vprint` only with an explicit
  bounded varargs/AAPCS stack-marshalling design;
- add persisted lifecycle/destructor/unload semantics;
- add broader relocation/TLS/IFUNC only when target evidence requires them;
- obtain real Android-device execution evidence when an environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
