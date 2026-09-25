# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `millesant/.gpt@2b7c8b9245a64560cc9e986d554d99233e34b8c5`.

`project-cleanup-v8` and its source/test, tooling, and documentation child changes are DONE. Features `011-elf32-combined-relocation-transaction`, `012-elf32-rel32-relocation`, `013-real-arm32-fixture-execution`, `014-elf32-symbol-versioning`, and `015-elf32-symbol-scope-policy` are DONE. Feature 015 result revision `2ed5157504dc9d7affac2290b1a19535b28913f9` passed exact-head CI run `36193971238` across Linux A32 smoke and both required Android lanes.

The next runtime feature should now address an accepted compatibility gap beyond requester-local scope ordering. The supplied ARM32 FMOD/VLC evidence continues to favor Android dependency/provider/search/global-group lifetime before speculative relocation expansion.

## Candidate runtime directions

Choose a bounded next feature from accepted gaps rather than continuing cosmetic repository churn. The supplied VLC ARMv7 four-DSO scan is recorded in `docs/research/evidence/vlc-armv7-gap-scan-2026-09-25.md`: every observed relocation is already inside the implemented REL/JUMP_SLOT set, while APK-external Android providers and INIT_ARRAY/FINI_ARRAY lifecycle metadata are concrete remaining requirements.

Current candidates include:

- Android search-path/namespace/link-map/global-group lifetime policy across independent graph loads;
- constructor/destructor lifecycle, beginning with bounded INIT_ARRAY semantics once the relevant graph/provider layer is available;
- broader ARM relocation coverage beyond REL32, such as COPY/instruction families or RELA/RELR/Android packed encodings, when a concrete target requires them;
- TLS/IFUNC groundwork when a concrete target requires it;
- end-to-end real ARM32 fixture execution on Android once the required runtime environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
