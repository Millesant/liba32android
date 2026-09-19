# Android runtime smoke — opt-in crash-test evidence

Date: 2026-09-19  
Environment: Termux on Android 16 / runtime SDK 36 / AArch64 / 4096-byte host pages  
CI artifact source: exact-head run #134 (`35436087356`) at `4cb17e059b8258f0cf8ce462c58012ab7d55249d`  
Merged feature baseline: PR #23 / `709e9ce74e58ee12d925b64c8466b9afa58cc4a6`  
Raw terminal transcript: `android-runtime-smoke-crash-test-termux-arm64-2026-09-19.log`

## Observed

- The ordinary runtime smoke completed successfully before the destructive test: `a32.return42.status=PASS`, `a32.memory.status=PASS`, `a32.memory.fastmem_direct=true`, and `runtime_smoke.complete=true`.
- The process reported Android release 16 / runtime SDK 36 on AArch64 with a 4096-byte host page size.
- The mapped-memory smoke obtained fastmem at host base `0x73fba44000`; mapped A32 data access used zero data callbacks.
- A separate `./run.sh --crash-test` invocation reported `diagnostics.crash_markers=enabled`.
- The destructive path emitted `crash_test.status=ARMED` and `crash_test.signal=SIGABRT` before termination.
- The installed handler emitted `A32CRASH|component=android_runtime_smoke|signal=6|pid=31431|addr=0x27db00007ac7`.
- The shell then reported `Aborted ./run.sh --crash-test`, directly observing fatal SIGABRT process termination after the marker.

## Inferred

- On this Termux/Android environment, the opt-in crash-test path reaches the runtime-smoke crash handler before the process terminates from SIGABRT.
- The explicit crash marker and normal fatal termination can coexist for the tested SIGABRT path without affecting an earlier ordinary smoke invocation.

## Not demonstrated

- An Android tombstone or native crash backtrace was not included in the captured evidence, so tombstone/backtrace coexistence remains NOT OBSERVED.
- The crash test does not demonstrate SIGSEGV/SIGBUS/SIGILL/SIGFPE marker behavior.
- This remains one Android/vendor/kernel environment and is not broad compatibility evidence.
- The host page size is 4096; actual 16 KiB Android host-page behavior remains NOT RUN.

## Consequence

The dedicated destructive mode now has real-device evidence for its core safety/diagnostic contract: ordinary smoke remains non-destructive, while the explicit crash invocation arms the test, emits the SIGABRT crash marker, and terminates the process. A separate Android tombstone/backtrace capture is still needed before claiming tombstone coexistence.
