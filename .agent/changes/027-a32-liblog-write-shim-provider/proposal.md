# Proposal — ARM32 guest liblog write shim/provider

## Intent

Join the verified ELF dependency/linker path to the verified feature-026 Android
log-write service with one real ARM32 guest compatibility DSO.

The supplied FMOD and VLC evidence both require `liblog.so` /
`__android_log_write`. Feature 026 implements the host-service semantics;
feature 027 supplies only the guest symbol/provisioning seam needed to reach it.

## Smallest shim

Generate a freestanding ARMv7 Android DSO with SONAME `liblog.so` and one
exported function:

```text
__android_log_write:
    svc #0xa0
    bx lr
```

The immediate comes from one shared preprocessor/C++ header so guest and host
cannot silently drift.

## Provider seam

Reuse the existing exact-name dependency catalog rather than inventing a second
provider type. A compatibility helper supplies the canonical requested name,
opaque identity, and borrowed image span for the generated shim.

## End-to-end fixture

A second freestanding ARM32 DSO imports `__android_log_write`, producing
`DT_NEEDED liblog.so` and eager `R_ARM_JUMP_SLOT`. The integration test loads
that consumer, acquires the shim through the catalog, relocates the call, maps
bounded guest data/stack storage, executes the consumer, crosses the real shim
SVC into features 025/026, and returns to the caller-selected stop PC.

## Non-goals

No embedded binary asset, automatic catalog installation, Android path/namespace
policy, print/vprint varargs, direct host liblog call, or broader platform API
surface.
