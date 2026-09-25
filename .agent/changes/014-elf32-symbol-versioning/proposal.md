# Proposal — ELF32 symbol versioning

## Intent

Implement the smallest version-aware symbol-resolution slice required by ordinary Android ARM32 DSOs before expanding global interposition or compatibility shims.

## Evidence motivating the slice

The supplied ARMv7 VLC APK contains DT_VERSYM plus VERNEED/VERDEF metadata in every inspected native DSO. Undefined Android platform imports are versioned with the name `LIBC`. The current runtime intentionally stops at `UnsupportedVersioning`, even though the relocation families used by these DSOs are already inside the accepted ARM REL/JUMP_SLOT subset.

## Boundary

This feature handles GNU/SysV dynamic symbol-version metadata and version-constrained graph-local resolution. It deliberately preserves the current breadth-first dependency scope.

It does not implement DT_SYMBOLIC/DF_SYMBOLIC, Android global groups/namespaces/preloads, process-wide link maps, TLS/IFUNC, constructors, dlopen/dlsym, or libc/JNI/graphics/audio compatibility.
