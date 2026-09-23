# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution`, `007-elf32-relocations`, and `008-elf32-plt-relocation-metadata` are DONE. Feature 008's final exact-head gate PASSed CI #223 / run `35934340806` at `79e9d8c90824baf76d7ff382661422af17e3cb6e`.

1. Next M4 linker/runtime slice — READY FOR SELECTION.
   - Preserve the completed feature-008 boundary: validated guest-only `DT_JMPREL` / `DT_PLTRELSZ` / `DT_PLTREL=DT_REL` metadata exists, but no PLT relocation entry is decoded or applied.
   - Deferred areas include eager `R_ARM_JUMP_SLOT` application, lazy binding/`DT_PLTGOT`, version-aware/protected requester semantics, Android/global-group/process-wide interposition policy, RELRO, TLS, IFUNC, and broader relocation forms. This list is not a priority ordering.
   - Exact next action: select one bounded M4 slice, establish its acceptance/evidence requirements, and readiness-check a new focused spec before implementation.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items into the next M4 feature unless its accepted scope explicitly requires them.
