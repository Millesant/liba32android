# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `millesant/.gpt@2b7c8b9245a64560cc9e986d554d99233e34b8c5`.

`project-cleanup-v8` and its source/test, tooling, and documentation child changes are DONE. Features `011-elf32-combined-relocation-transaction` through `017-elf32-lifecycle-array-metadata` are DONE. Feature 017 result revision `e380b96f4e5d2c81a471d051f568b7c2dbef2c1c` passed exact-head Linux A32 smoke and both required Android checks.

Feature `019-elf32-init-call-execution` is ACTIVE. It adds a generic CPU stop-PC termination condition and uses it to invoke feature-018 constructor calls with ARM/Thumb state derived from each raw function value, a caller-owned 8-byte-aligned guest stack top, and a per-call instruction ceiling. No scratch mapping, legacy INIT/PREINIT, persisted called-state, or destructor lifecycle is absorbed.

## Candidate runtime directions

Choose a bounded next feature from accepted gaps rather than continuing cosmetic repository churn. The supplied VLC ARMv7 four-DSO scan is recorded in `docs/research/evidence/vlc-armv7-gap-scan-2026-09-25.md`: every observed relocation is already inside the implemented REL/JUMP_SLOT set, while APK-external Android providers and INIT_ARRAY/FINI_ARRAY lifecycle metadata are concrete remaining requirements.

Current candidates include:

- layer Android namespace/search-path/platform-provider policy above the persistent link-map provider boundary;
- complete feature 019 bounded guest constructor invocation, then decide between Android platform-provider/search policy and FINI/destructor lifecycle;
- broader ARM relocation coverage beyond REL32, such as COPY/instruction families or RELA/RELR/Android packed encodings, when a concrete target requires them;
- TLS/IFUNC groundwork when a concrete target requires it;
- end-to-end real ARM32 fixture execution on Android once the required runtime environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
