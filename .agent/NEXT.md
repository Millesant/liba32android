# Next Work

Repository integration is on `bleeding`. The current control-plane round is pinned to `millesant/.gpt@2b7c8b9245a64560cc9e986d554d99233e34b8c5`.

`project-cleanup-v8` and its source/test, tooling, and documentation child changes are DONE. Features `011-elf32-combined-relocation-transaction`, `012-elf32-rel32-relocation`, `013-real-arm32-fixture-execution`, and `014-elf32-symbol-versioning` are DONE; feature 014 implementation revision `5ba659dbf3ad9328c8e46af4441db3a0c4bb4a26` passed Linux A32 smoke and both Android required checks.

Feature `015-elf32-symbol-scope-policy` is ACTIVE. The bounded implementation is present: validated DT_SYMBOLIC/DF_SYMBOLIC metadata, caller-owned in-graph global-scope ordering for relocation/reference lookup, requester-first symbolic binding, and focused metadata/symbol/relocation tests. This slice was selected from direct inspection of the supplied ARM32 `libfmod.so` and VLC `libvlc.so`, both of which advertise symbolic binding. Exact-head CI has not yet surfaced, so the immediate next step is verification/convergence rather than another semantic expansion.

## Candidate runtime directions

Choose a bounded next feature from accepted gaps rather than continuing cosmetic repository churn. The supplied VLC ARMv7 four-DSO scan is recorded in `docs/research/evidence/vlc-armv7-gap-scan-2026-09-25.md`: every observed relocation is already inside the implemented REL/JUMP_SLOT set, while APK-external Android providers and INIT_ARRAY/FINI_ARRAY lifecycle metadata are concrete remaining requirements.

Current candidates include:

- after feature 015 verification, Android search-path/namespace/link-map/global-group lifetime policy across independent graph loads;
- constructor/destructor lifecycle, beginning with bounded INIT_ARRAY semantics once the relevant graph/provider layer is available;
- broader ARM relocation coverage beyond REL32, such as COPY/instruction families or RELA/RELR/Android packed encodings, when a concrete target requires them;
- TLS/IFUNC groundwork when a concrete target requires it;
- end-to-end real ARM32 fixture execution on Android once the required runtime environment is available.

## Independent follow-ups

- Android native tombstone/backtrace coexistence: BLOCKED on accessible device environment.
- AArch64 runtime execution on a 16 KiB Android host: NOT RUN.
- Project license: BLOCKED on maintainer choice.
