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

`android_address_space_probe` prints machine-readable `key=value` lines. On an OS/API failure it also prints the numeric errno and the corresponding text.

Example shape:

```text
android.api=26
arch=aarch64
kernel.release=...
fastmem4g.status=failed
fastmem4g.errno=12
fastmem4g.error=Cannot allocate memory
```

When sending probe output, send the complete output if practical. At minimum include:

- `android.api`
- `arch`
- `kernel.release`
- `page_size`
- `mmap_min_addr.*`
- every section whose `.status` is not `ok`/`pass`
- its matching `.errno` and `.error` lines
- the generated-code result if `--execute-generated-code` was used

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

## Android collection

Once runtime logging is wired to Android, the desired collection path is one command whose output can be pasted directly. Until that integration exists, the standalone probe can be run from `/data/local/tmp` and its stdout copied verbatim.

For future runtime failures, preserve the first `A32ERR|...` line and any immediately preceding/following context. A crash tombstone or native backtrace should be sent in addition to, not instead of, the structured error line.

## What to send here

The best evidence, in priority order:

1. GitHub Actions run link for CI failures;
2. complete probe output for Android mapping/JIT probes;
3. future `A32ERR|...` line(s) plus nearby context;
4. native crash/tombstone/backtrace if the process dies before structured reporting.

Do not redact guest addresses or PCs unless they contain information you intentionally need to keep private; those values are often necessary to reproduce loader, memory and translation faults.
