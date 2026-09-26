# ARM32 guest `liblog.so` write shim/provider path

Status: feature 027 implementation prepared; exact-head validation NOT RUN

## Goal

Connect the already-verified feature-026 host service to the existing ELF
dependency/symbol/relocation machinery with the smallest real ARM32 guest
compatibility library.

The feature supplies reproducible source for a freestanding ARMv7 Android
`liblog.so` whose only exported compatibility function is
`__android_log_write`.

## Private guest/host protocol

`src/compat/a32_android_log_shim.h` is the single source of truth for the
private service immediate:

`LIBA32ANDROID_A32_ANDROID_LOG_WRITE_SHIM_SVC == 0xA0`.

The guest assembly fixture includes that header through the C preprocessor, and
host integration code uses the matching C++ constant
`kA32AndroidLogWriteShimSvcImmediate`.

The guest function is deliberately tiny:

```text
__android_log_write:
    svc #0xa0
    bx lr
```

It does not marshal or rewrite arguments. AAPCS32 therefore leaves feature 026
to consume r0/r1/r2 exactly as established by its accepted contract.

## ELF/provider boundary

The reproducible shim is linked with SONAME `liblog.so`. The companion
freestanding consumer has one `DT_NEEDED` edge to `liblog.so` and one
`R_ARM_JUMP_SLOT` reference to `__android_log_write`.

`make_a32_android_log_shim_catalog_entry` creates the exact-name catalog entry
for that SONAME using stable identity `liba32android-compat-liblog`. The entry
borrows the supplied image bytes; neither the helper nor
`Elf32DependencyCatalogProvider` owns the backing storage.

The generated DSO is not vendored into Git and is not embedded into
`liba32android.so`. Packaging an ARM32 shim image remains an embedding/build
responsibility.

## Integration path

The feature-027 integration regression:

1. loads the real consumer root;
2. resolves `DT_NEEDED liblog.so` through the exact-name catalog helper;
3. resolves `__android_log_write` from the loaded shim object;
4. applies the consumer's eager JUMP_SLOT relocation;
5. maps bounded guest stack/string storage;
6. executes the consumer function;
7. crosses the real shim SVC into the feature-025 registry and feature-026 log
   service;
8. resumes through `bx lr` and reaches the requested stop PC.

This is the first proof that the ELF linker side and compatibility-service side
are connected by a real ARM32 guest library rather than only a synthetic SVC
program.

## Deliberate exclusions

The shim exports no `__android_log_print` or `__android_log_vprint`; there is
no varargs/stack marshalling, Android namespace/search policy, automatic
platform-catalog installation, host `liblog` binding, JNI/graphics/audio
surface, or broad application compatibility claim.
