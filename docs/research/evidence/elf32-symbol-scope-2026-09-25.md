# ELF32 requester/global symbol-scope evidence — 2026-09-25

## Purpose

Record the external ARM32 binaries that motivated feature 015 without adding
third-party binaries to the repository. These observations establish that
`DT_SYMBOLIC` / `DF_SYMBOLIC` occur in supplied Android ARMv7 DSOs and that
their observed relocation families are already inside LibA32Android's current
REL/JUMP_SLOT implementation boundary.

This is binary-inspection evidence only. It is not runtime-execution or CI
evidence.

## Inputs

- supplied `libfmod.so`: ELF32 little-endian ARM EABI5 shared object;
- supplied VLC Android 3.7.2 Beta 2 APK:
  `lib/armeabi-v7a/libvlc.so`, extracted only for local inspection.

The binaries are not vendored, committed, or redistributed by this project.

## Dynamic-tag observations

`readelf -d libfmod.so` reports both:

```text
(SYMBOLIC) 0x0
(FLAGS)    SYMBOLIC BIND_NOW
```

`readelf -d libvlc.so` reports the same symbolic-binding pair:

```text
(SYMBOLIC) 0x0
(FLAGS)    SYMBOLIC BIND_NOW
```

The VLC ARMv7 DSO also carries `VERSYM`, `VERDEF/VERDEFNUM`, and
`VERNEED/VERNEEDNUM`, so requester/global ordering must compose with the
already-implemented bounded version filter rather than bypass it.

## Relocation inventory

Observed `R_ARM_*` relocation counts from `readelf -r`:

| DSO | RELATIVE | ABS32 | GLOB_DAT | JUMP_SLOT |
| --- | ---: | ---: | ---: | ---: |
| `libfmod.so` | 1722 | 6 | 3 | 98 |
| VLC ARMv7 `libvlc.so` | 72416 | 1134 | 97 | 492 |

No additional relocation family appeared in these two inspected DSOs. This
does not claim that all VLC dependencies or Android applications are covered;
it only narrows the compatibility gap demonstrated by these supplied samples.

## Feature consequence

Feature 015 therefore keeps relocation semantics unchanged and adds only the
missing lookup-order boundary:

- retain `DT_SYMBOLIC` and `DF_SYMBOLIC` in validated linker metadata;
- allow relocation/reference lookup to receive an ordered in-graph global
  candidate list;
- search ordinary requesters as global then graph-local scope;
- search symbolic requesters as self then global then remaining local scope;
- apply the existing version filter and object-count ceiling across the same
  ordered candidate set.

Feature 016 now supplies persistent cross-root link-map lifetime and generic
global-group membership/order. Android namespace/search-path policy, preloads,
RTLD semantics, and platform-provider selection remain separate future work.
