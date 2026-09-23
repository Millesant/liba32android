# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is DONE. Feature `007-elf32-relocations` is active.

1. T001 — bounded read-only main-`DT_REL` decoding/planning.
   - Status: ACTIVE.
   - Base revision for specification/readiness: `ecdae1cea991ecd079487e1cda0bc2fe3c7fff99`.
   - Accepted first relocation set: `R_ARM_NONE`, `R_ARM_ABS32`, `R_ARM_GLOB_DAT`, `R_ARM_RELATIVE`.
   - Android compatibility decision: `R_ARM_GLOB_DAT` will write `S` and ignore the REL in-place addend, matching current bionic.
   - Pinned real-fixture oracle: CI #208 artifact ID `10745105004` contains exactly two `R_ARM_GLOB_DAT` entries at linked offsets `0x82cc` / `0x82d0`, symbol indexes 2 / 3 (`fixture_bss` / `fixture_data`), with zero original words.
   - Implemented scope for the pending gate: `src/elf/elf32_relocation.{h,cpp}`, explicit bounded REL decoding, checked guest-place calculation, supported-type classification, original-word capture, duplicate-target rejection, synthetic coverage, CMake wiring, and pinned real-fixture read-only plan coverage.
   - Exact next action: run exact-head CI and require `elf32_relocation_plan`, `elf32_real_relocation_plan`, all neighboring ELF/linker/symbol tests, Android x86_64, and Android arm64-v8a to PASS.
   - Stop condition: T001 plan/decode behavior is validated and persisted. Do not begin symbol-resolution/application writes until the T001 gate is green.

2. T002-T005 remain dependency-ordered in `specs/007-elf32-relocations/tasks.md`.
   - Symbol-reference resolution, transactional writes/rollback, real-fixture application, and final convergence are not admitted into the T001 round.

Independent FOLLOW_UP items remain unchanged:

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.

Do not fold these independent items into feature 007 unless a relocation acceptance criterion makes one REQUIRED_NOW.
