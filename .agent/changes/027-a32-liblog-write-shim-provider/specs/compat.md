# Compatibility spec delta — feature 027

Define one shared private SVC immediate, `0xA0`, for the first guest ARM32
Android-log compatibility shim. Guest assembly and host C++ consume the same
header definition.

Provide reproducible freestanding ARMv7 source for `liblog.so` exporting only
`__android_log_write` as an ARM function that traps with that SVC and returns
with `bx lr`. Provide a companion real ARM32 consumer that creates
`DT_NEEDED liblog.so` and an eager `R_ARM_JUMP_SLOT` import.

Expose a helper that creates a borrowed exact-name
`Elf32DependencyCatalogEntry` for `liblog.so` with stable opaque identity.
No shim image ownership or automatic provider installation is added.

Integration must prove real dependency loading, symbol resolution, relocation,
and execution through the feature-025 registry and feature-026 service using
bounded guest data/stack mappings and the existing requested-stop runtime
contract.

No print/vprint varargs, Android namespace/search policy, host liblog binding,
binary vendoring, JNI/graphics/audio behavior, or general application
compatibility is part of this change.
