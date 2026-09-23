# Next Work

Repository integration is on `bleeding`. Features `006-elf32-symbol-resolution` and `007-elf32-relocations` are DONE. Feature 007's final gate PASSed exact-head CI #218 / run `35918899544` at `8efe792cfa58a3f34e02dfe0c8bb01fbc3949766`.

1. Next M4 linker/runtime slice — READY FOR SELECTION.
   - Start a new focused `specs/<id>-<feature>/` requirements/design/tasks package before feature-scale implementation.
   - Preserve the completed feature-007 boundary: the current main-`DT_REL` NONE/RELATIVE/GLOB_DAT/ABS32 contract is validated and must not be silently broadened.
   - Deferred areas include PLT/JMPREL, version-aware/protected requester semantics, Android/global-group/process-wide interposition policy, RELRO, TLS, IFUNC, and broader relocation forms. This list is not a priority ordering.
   - Exact next action: select one bounded M4 slice, establish its acceptance/evidence requirements, and readiness-check the new spec before implementation.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items into the next M4 feature unless its accepted scope explicitly requires them.
