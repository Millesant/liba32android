# VLC ARMv7 compatibility-gap scan — 2026-09-25

## Scope

This is a bounded static scan of the four `lib/armeabi-v7a/*.so` entries in the
supplied VLC Android 3.7.2 Beta 2 APK. The APK and extracted binaries remain
external inputs and are not committed to this repository.

The purpose is to rank post-feature-015 work using observed ELF requirements,
not to claim VLC compatibility.

## Native set

The APK contains four ARMv7 DSOs:

- `libc++_shared.so`
- `libmla.so`
- `libvlc.so`
- `libvlcjni.so`

## Relocation families

Aggregating `readelf -r` across those four DSOs produced only relocation
families already implemented by the current bounded REL/JUMP_SLOT layer:

| Relocation | Observed count |
| --- | ---: |
| `R_ARM_RELATIVE` | 76047 |
| `R_ARM_JUMP_SLOT` | 14428 |
| `R_ARM_ABS32` | 7798 |
| `R_ARM_GLOB_DAT` | 1021 |

No `RELA`, `RELR`, Android packed relocation dynamic tags, TLS segment, or
dynamic `IFUNC` symbol was observed in this four-DSO sample.

This does not prove those forms are unnecessary generally; it only means that
broader relocation/TLS/IFUNC work is not the next blocker demonstrated by this
specific VLC sample.

## Lifecycle metadata

Lifecycle metadata is present:

| DSO | INIT_ARRAY | FINI_ARRAY |
| --- | ---: | ---: |
| `libc++_shared.so` | yes | yes |
| `libmla.so` | yes | yes |
| `libvlc.so` | yes | yes |
| `libvlcjni.so` | no | yes |

None of the four exposes a legacy `DT_INIT` or `DT_FINI` entry in this
sample.

## Dependency/provider boundary

The APK-local graph is not self-contained. Examples of external Android
dependencies include `libc.so`, `libdl.so`, `libm.so`, `liblog.so`,
`libandroid.so`, `libEGL.so`, and `libGLESv2.so`. Undefined imports also
include pthread/libc/libdl routines, Android logging, EGL/GLES entry points,
and C++ ABI functions.

That makes provider/search/global-group construction and future compatibility
surfaces concrete prerequisites for loading these DSOs as an application
graph. Feature 015 intentionally supplies only requester/global lookup ordering
inside one already-loaded graph; it does not construct Android namespaces or
platform-library providers.

## Planning consequence

After feature 015 is verified, the supplied VLC sample currently points to two
nearer compatibility gaps than adding more relocation encodings:

1. Android-oriented dependency/provider/search/namespace policy across
   application and platform objects. Feature 016 now supplies generic
   persistent link-map/global-group lifetime beneath that policy.
2. Constructor/destructor lifecycle, beginning with bounded `DT_INIT_ARRAY`
   semantics once the relevant dependency/provider graph can be assembled.

Both remain separate features and require their own accepted contract before
implementation.
