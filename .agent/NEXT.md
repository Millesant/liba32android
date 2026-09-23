# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is DONE. Feature `007-elf32-relocations` is active.

1. T002 — bounded relocation-reference symbol decoding/resolution.
   - Status: ACTIVE.
   - Dependency gate: T001 PASSed exact-head CI #210 / run `35850236188` at `2b4e9185bac43fe9bb46ddf8c7da9b73e0146837`; Linux passed 43/43 CTest including `elf32_relocation_plan` and `elf32_real_relocation_plan`, Android x86_64 PASSed, and Android arm64-v8a PASSed.
   - Existing T001 contract: main-`DT_REL` is decoded read-only with explicit count bounds, checked guest places, supported-type classification, original-word snapshots, and duplicate write-target rejection.
   - Implemented scope for the pending gate: additive bounded dynsym-entry read helper; relocation-reference index/name decoding; GLOBAL/WEAK DEFAULT reference policy; protected/versioned/TLS/IFUNC/common/XINDEX rejection; graph-local BFS resolution; strong-not-found failure; unresolved-weak S=0; nested error preservation; synthetic and real-fixture read-only coverage.
   - Exact next action: run exact-head CI and require `elf32_relocation_plan`, `elf32_real_relocation_plan`, all neighboring symbol/linker tests, Android x86_64, and Android arm64-v8a to PASS.
   - Stop condition: T002 resolution behavior is validated and persisted. Do not begin transactional relocation writes until the T002 gate is green.

2. T003-T005 remain dependency-ordered in `specs/007-elf32-relocations/tasks.md`.
   - Transactional writes/rollback, real-fixture application, and final convergence are not admitted into T002 until its dependency gate passes.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items into feature 007 unless a relocation acceptance criterion makes one REQUIRED_NOW.
