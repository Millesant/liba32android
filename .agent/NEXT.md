# Next Work

Repository integration is on `bleeding`. The current control-plane round is
pinned to
`millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

Features 011 through 027 are DONE. The latest behavior-changing result is
feature 027 at `37624f389bb34197669a763c32631a1174d099a6`, with the
dedicated ARM32 liblog shim integration, Linux A32 smoke, and both Android
checks PASSed.

The runtime can now load a real consumer that needs `liblog.so`, acquire the
generated shim through the exact-name catalog, relocate
`__android_log_write`, execute through the guest SVC and feature-026 service,
and return successfully.

## Candidate runtime directions

The next dependency boundary is no longer the log-write symbol itself; it is how
platform compatibility libraries are selected and made accessible to requesters.

Current bounded candidates are:

- introduce the smallest explicit platform-library catalog/selection policy
  above the existing requester-aware provider chain, starting with the generated
  `liblog.so` shim and preserving exact requester context;
- then add Android namespace/accessibility/search semantics only where concrete
  application/platform evidence requires them;
- add `__android_log_print`/`__android_log_vprint` only with a bounded
  varargs/AAPCS stack-marshalling design;
- add persisted lifecycle/destructor/unload semantics;
- add broader relocation/TLS/IFUNC only when target evidence requires them;
- obtain real Android-device execution evidence when an environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
