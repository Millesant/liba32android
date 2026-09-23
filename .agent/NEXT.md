# Next Work

Repository integration is on `bleeding`. Feature `006-elf32-symbol-resolution` is DONE.

1. Next feature-scale M4 linker slice — ARM relocation application.
   - Status: READY FOR SPECIFICATION; implementation has not started.
   - Dependency basis: feature 006 final T005 gate PASSed exact-head CI #207 / run `35847914558` at `ad022c2cc569c3175ad1cef0140f964817f5a820`; Linux passed 41/41 CTest including `elf32_real_symbol_lookup`, Android x86_64 PASSed, and Android arm64-v8a PASSed.
   - Exact next action in a fresh bounded round: verify the next unused feature ID, then create a focused `specs/<next-id>-elf32-relocations/{requirements,design,tasks}.md` package before implementation.
   - Initial boundary: consume the validated REL metadata, loaded dependency graph, and graph-local symbol lookup contract; keep version matching, Android global-group/namespace policy, TLS, IFUNC execution, PLT/lazy binding, and unrelated compatibility layers outside the first relocation slice unless the new requirements prove one is REQUIRED_NOW.

2. Remaining independent evidence/policy gaps.
   - Android native tombstone/backtrace coexistence remains BLOCKED on an accessible device environment.
   - AArch64 runtime execution on a 16 KiB Android host remains NOT RUN.
   - Project license remains BLOCKED on maintainer choice.

Do not fold these independent follow-ups into the relocation feature unless their own requirements become dependencies.
