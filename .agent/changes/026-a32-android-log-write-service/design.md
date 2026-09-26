# Design — A32 Android log-write service

## Layer

Add `src/compat/a32_android_log_write.*` in namespace
`liba32android::compat`.

`A32AndroidLogWriteService` implements the generic
`runtime::A32HostServiceHandler` interface and contains no Dynarmic or ELF
types. The caller selects the private SVC immediate, allowing future guest shim
selection to remain a separate concern.

## Sink

`A32AndroidLogSink` is caller-owned and synchronously receives:

- signed 32-bit priority;
- optional tag view where `nullopt` preserves a null guest tag;
- text view.

Views point into service-owned temporary string copies and are valid only for
that call. The sink returns signed 32-bit status; the service copies its exact
bit pattern to guest r0.

## AAPCS mapping

For the matching service ID:

- r0 -> signed 32-bit `prio`;
- r1 -> logical guest `tag` pointer;
- r2 -> logical guest `text` pointer.

Other registers and CPSR are untouched by the bridge itself.

## Bounded C-string read

A private helper reads one guest byte at a time through `GuestMemory` to keep
permission/fault behavior identical across memory backends.

The caller-supplied limit counts payload bytes excluding NUL. For each offset:

1. reject logical 32-bit address overflow;
2. read exactly one byte;
3. NUL -> success;
4. if the current offset equals the payload ceiling and the byte is non-NUL,
   fail;
5. otherwise append the byte and continue.

This accepts empty strings and payloads exactly at the configured maximum while
never scanning beyond the explicit limit.

## Null/failure policy

A zero tag pointer bypasses guest memory reads and becomes `nullopt` for the
sink. A zero text pointer is rejected. Any non-null read failure, address
overflow, or missing terminator returns `Failed` before sink invocation.

A mismatched SVC immediate returns `Unhandled`.

## Composition regression

The main test stages an ARM `svc; bx lr` sequence plus guest tag/text data,
constructs feature-026 service -> feature-025 registry -> feature-024 dispatcher,
and verifies successful return-to-stop with the sink result in r0. Focused
direct cases cover null tag, exact bounds, mismatch, null text, unreadable tag,
and over-limit unterminated text.

## Non-goals

No host liblog dependency, guest ELF shim generation, dependency-provider
selection, namespace policy, or varargs/stack marshalling.
