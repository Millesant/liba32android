# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `millesant/.gpt@f4e926e81ad91d13d02a006f4a18a00f66ae0bab`.

`project-cleanup-v8` and its source/test, tooling, and documentation child changes are DONE. Features `011-elf32-combined-relocation-transaction` through `019-elf32-init-call-execution` are DONE. Feature 019 result revision `28a4f92f78b8ff156ee9dd3083d1218af8b7b125` passed exact-head Linux A32 smoke and both required Android checks.

Features 020 and 021 are DONE. Feature 021 result revision `4324883faef008810bcf77c390eecd92c16718cd` passed exact-head Linux A32 smoke and both required Android checks. Feature `022-elf32-dependency-catalog-provider` is ACTIVE: add an exact-byte-name in-memory catalog provider that returns owned dependency sources from caller-owned entries and composes directly with the verified provider chain.

## Candidate runtime directions

Choose a bounded next feature from accepted gaps rather than continuing cosmetic repository churn. The supplied VLC ARMv7 four-DSO scan is recorded in `docs/research/evidence/vlc-armv7-gap-scan-2026-09-25.md`: every observed relocation is already inside the implemented REL/JUMP_SLOT set, while APK-external Android providers and INIT_ARRAY/FINI_ARRAY lifecycle metadata are concrete remaining requirements.

Current candidates include:

- layer Android namespace/search-path/platform-provider policy above the persistent link-map provider boundary;
- complete feature 022 exact-name dependency catalogs, then use separate application/platform catalogs to model the supplied VLC/FMOD dependency shapes before introducing filesystem/APK search policy;
- broader ARM relocation coverage beyond REL32, such as COPY/instruction families or RELA/RELR/Android packed encodings, when a concrete target requires them;
- TLS/IFUNC groundwork when a concrete target requires it;
- end-to-end real ARM32 fixture execution on Android once the required runtime environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
