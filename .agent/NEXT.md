# Next Work

Repository integration is on `bleeding`. The current control-plane round is
pinned to
`millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

Features 011 through 027 are DONE.

`028-a32-android-platform-provider` is IMPLEMENTED. It adds the first concrete
requester-aware platform-library policy seam for the generated `liblog.so`
shim, application-first/platform-fallback provider composition, and real
integration coverage proving the original requester identity reaches platform
policy. Exact-head Linux A32 smoke, dedicated liblog integration, and both
Android checks are NOT RUN and are the immediate acceptance gate.

After feature 028 acceptance, the next bounded decision is whether observed
targets require an actual Android namespace/accessibility model or another
concrete platform shim first. Current evidence favors namespace/accessibility
research before adding arbitrary library names because FMOD/VLC already mix
APK-local and multiple Android platform dependencies.

Other follow-ups remain:

- `__android_log_print`/`__android_log_vprint` only with bounded varargs/AAPCS
  stack marshalling;
- persisted destructor/unload lifecycle;
- broader relocation/TLS/IFUNC only when target evidence requires;
- real Android-device execution evidence.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
