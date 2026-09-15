# Diagnostics and error sharing

Status: current CI/probe behavior documented; runtime-wide structured errors are a contract for future public runtime/API work.

## Goal

When something fails on a device, emulator, CI runner, or future embedding application, the useful evidence should be copy-pasteable into an issue or chat without requiring interpretation first.

Do not replace the original failure text with a paraphrase. Keep addresses, errno values, PCs, symbols and component names intact.

## Current GitHub Actions failures

For GitHub CI, the preferred report is simply the Actions run URL. The repository workflow keeps named steps and uses `ctest --output-on-failure`, so the run URL is enough for a maintainer/agent with repository access to inspect the exact failure.

If sharing text instead of a URL, copy:

1. the failed step name;
2. the first explicit error/fatal line;
3. approximately 20 lines before and after it;
4. the final command exit status when shown.

## Current Android address-space probe

`android_address_space_probe` prints machine-readable `key=value` lines and mirrors the same output into a file when file logging is available. On an OS/API failure it also prints the numeric errno and the corresponding text.

Example shape:

```text
diagnostics.file_log=enabled
diagnostics.log_file=/sdcard/Download/liba32android/address-space-probe.log
android.api=26
arch=aarch64
kernel.release=...
fastmem4g.status=failed
fastmem4g.errno=12
fastmem4g.error=Cannot allocate memory
```

When sending probe output, send the complete output if practical. At minimum include:

- `diagnostics.*`
- `android.api`
- `arch`
- `kernel.release`
- `page_size`
- `mmap_min_addr.*`
- every section whose `.status` is not `ok`/`pass`
- its matching `.errno` and `.error` lines
- the generated-code result if `--execute-generated-code` was used

## File logging on Android

The standalone probe now tries to create a user-visible log automatically. Selection order is:

1. `/sdcard/Download/liba32android/address-space-probe.log`;
2. `$HOME/liba32android/address-space-probe.log` when `HOME` is defined;
3. `./liba32android-address-space-probe.log` in the current working directory.

The exact selected location is always printed as:

```text
diagnostics.log_file=<path>
```

The first path is intentionally aimed at the Android **Downloads** area so a probe launched through `adb shell` can usually leave a file that is easy to find with a file manager. If Android denies that shared-storage path, the probe does not treat that as a mapping/JIT failure; it reports the errno and falls back.

You can force an exact location with either:

```text
android_address_space_probe --log-file /some/path/probe.log
```

or:

```text
LIBA32ANDROID_LOG_FILE=/some/path/probe.log android_address_space_probe
```

Use `--no-log-file` only when stdout/stderr are being captured by another tool.

Important Android boundary: an executable launched as the `adb shell` user and a normal application process do not have the same storage access. Android scoped storage limits direct shared-storage access for apps targeting modern Android. The future embedded runtime therefore must accept an app-provided/app-specific diagnostic directory (or an explicit export path) instead of assuming that every app can write `/sdcard/Download` directly.

Android storage references:

- https://developer.android.com/training/data-storage
- https://developer.android.com/training/data-storage/shared/documents-files

## Crash marker

The standalone probe installs minimal handlers for `SIGABRT`, `SIGBUS`, `SIGFPE`, `SIGILL`, and `SIGSEGV`. Before Android performs its normal fatal-signal handling, the probe attempts to write an async-signal-safe marker to stderr and to the already-open diagnostic file:

```text
A32CRASH|component=android_address_space_probe|signal=11|pid=12345|addr=0xdeadbeef
```

This marker is deliberately small: signal handlers cannot safely run normal C++ logging, allocate memory, symbolize stacks, or perform complex file operations. The marker supplements Android's normal tombstone/native backtrace; it does **not** replace it.

Because the log file is opened before the risky generated-code probe and is unbuffered, already-produced diagnostics plus the crash marker have a chance to survive even if generated code faults.

Device execution of this crash path is still **NOT RUN** until an Android arm64 environment executes the probe.

## Simple adb collection flow

After downloading the `android-address-space-probe-<sha>` artifact from GitHub Actions:

```text
adb push android_address_space_probe /data/local/tmp/
adb shell chmod 755 /data/local/tmp/android_address_space_probe
adb shell /data/local/tmp/android_address_space_probe
```

For the generated-code test:

```text
adb shell /data/local/tmp/android_address_space_probe --execute-generated-code
```

If the probe prints the default Downloads path, the resulting file should be:

```text
/sdcard/Download/liba32android/address-space-probe.log
```

and can also be copied to a computer with:

```text
adb pull /sdcard/Download/liba32android/address-space-probe.log .
```

If that path is unavailable, use the `diagnostics.log_file=` line to locate the fallback file.

## Runtime structured error format

The public runtime is not implemented yet, so runtime-wide text emission is **NOT IMPLEMENTED**. The intended stable text format is:

```text
A32ERR|component=<component>|code=<stable_code>|pc=<guest_pc>|addr=<guest_addr>|message=<human text>
```

Fields that do not apply may be omitted. Examples of intended future output:

```text
A32ERR|component=memory|code=GUEST_MEMORY_FAULT|pc=0x00104210|addr=0xdeadbeef|message=read from unmapped guest address
A32ERR|component=elf|code=ELF_BAD_MACHINE|message=expected EM_ARM
A32ERR|component=linker|code=UNRESOLVED_SYMBOL|message=symbol foo was not found
```

The stable `code` is for diagnosis/automation; the `message` is for humans. Guest addresses and guest PC must be printed in hexadecimal when available.

## Android collection for the future embedded runtime

The standalone probe now has automatic file logging, but the generic shared library still has no public logging API. Once that API exists, the host app should provide a writable directory. Preferred default storage for embedded use is an app-specific directory; explicit user export to Downloads should go through the host app's Android storage integration rather than being hard-coded inside the generic runtime.

For future runtime failures, preserve the first `A32ERR|...` line and any immediately preceding/following context. A crash tombstone or native backtrace should be sent in addition to, not instead of, the structured error line.

## What to send here

The best evidence, in priority order:

1. GitHub Actions run link for CI failures;
2. the complete `address-space-probe.log` file or its pasted contents;
3. any `A32CRASH|...` marker plus the Android tombstone/backtrace if the process crashed;
4. future `A32ERR|...` line(s) plus nearby context.

Do not redact guest addresses or PCs unless they contain information you intentionally need to keep private; those values are often necessary to reproduce loader, memory and translation faults.
