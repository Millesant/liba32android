# ELF32 lifecycle-array evidence — 2026-09-25

## Supplied VLC ARMv7 observations

Static inspection of the four `lib/armeabi-v7a/*.so` entries in the supplied
VLC Android 3.7.2 Beta 2 APK found:

| DSO | INIT_ARRAY | FINI_ARRAY |
| --- | ---: | ---: |
| `libc++_shared.so` | yes | yes |
| `libmla.so` | yes | yes |
| `libvlc.so` | yes | yes |
| `libvlcjni.so` | no | yes |

None of the four sample DSOs exposes legacy `DT_INIT` or `DT_FINI`.

The APK remains external test evidence and is not committed or redistributed.

## Android linker semantics used as later-lifecycle evidence

Android bionic's `soinfo::call_constructors` recursively calls children before
the object itself, then invokes `DT_INIT` before `DT_INIT_ARRAY`.
`call_destructors` traverses `DT_FINI_ARRAY` in reverse order before
`DT_FINI`. The shared call helper skips null and all-ones function pointers.

Primary source:
https://android.googlesource.com/platform/bionic/+/master/linker/linker_soinfo.cpp

Feature 017 intentionally adopts only the metadata/range/decoding boundary.
Ordering across objects, sentinel filtering, and guest function execution are
deferred so they can be specified together with recursion guards and process
argument/environment state.
