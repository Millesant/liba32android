# ELF32 lifecycle arrays

Status: feature 017 metadata/decoder boundary complete; exact-head implementation CI PASSed

## Boundary

Feature 017 introduces a non-executing lifecycle layer. Dynamic linker metadata
recognizes paired `DT_INIT_ARRAY/DT_INIT_ARRAYSZ` and
`DT_FINI_ARRAY/DT_FINI_ARRAYSZ` declarations and exposes validated guest-only
descriptors. `elf32_lifecycle` then decodes those descriptors into raw logical
32-bit function values under a caller-selected entry ceiling.

The boundary intentionally stops before Android constructor/destructor
orchestration. It does not decide dependency order, filter sentinels, track
constructor-called recursion state, or invoke the CPU.

## Metadata invariants

Each array tag and its size tag form a unique all-or-nothing pair. Duplicate
recognized singleton tags fail. Byte sizes must be divisible by four. The
address is rebased exactly once through the object's load bias, the complete
declared range must fit the 32-bit guest address space, and every non-empty byte
must be readable through `GuestMemory`.

Zero-length arrays are retained as descriptors and consume no guest bytes.

## Raw decoder

`decode_elf32_function_array`:

- rejects non-integral byte sizes;
- rejects arrays whose entry count exceeds `max_entries` before any read;
- rejects guest-range overflow;
- reads one little-endian 32-bit entry at a time through `GuestMemory`;
- returns declaration order unchanged;
- preserves raw `0` and `0xffffffff` values;
- returns no partial entry vector on failure;
- never changes guest bytes, mappings, or permissions.

Preserving sentinel values is deliberate. Android bionic's later call policy
skips null/all-ones function pointers, but filtering belongs with execution
semantics rather than metadata decoding.

## Android evidence and deferred ordering

The supplied VLC ARMv7 APK contains INIT_ARRAY in three of four inspected DSOs
and FINI_ARRAY in all four. Bionic lifecycle code calls dependency
constructors before the current object, invokes legacy `DT_INIT` before
INIT_ARRAY, traverses FINI_ARRAY in reverse order before `DT_FINI`, and
suppresses null/all-ones call targets.

Feature 017 adopts only the metadata and raw-decoding prerequisites. Legacy
`DT_INIT/DT_FINI`, PREINIT_ARRAY, dependency-order lifecycle planning,
recursion guards, guest CPU invocation, process argv/envp state, dlopen
lifecycle, and unload/destructor orchestration remain separate contracts.

## Validation

At implementation revision `e380b96f4e5d2c81a471d051f568b7c2dbef2c1c`:

- Linux A32 smoke check `108292909594` PASSed;
- Android x86_64 address-space probe check `108292909697` PASSed;
- Android arm64-v8a cross-build check `108292909672` PASSed.

Focused unit coverage proves exact decoding, raw sentinel preservation,
entry-ceiling preflight, malformed-size and range rejection, unreadable and
zero-length arrays, metadata duplicate/incomplete handling, rebasing/range
validation, and no guest mutation.
