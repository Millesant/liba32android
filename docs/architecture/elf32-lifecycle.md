# ELF32 lifecycle arrays

Status: feature 018 planning complete; feature 019 bounded INIT_ARRAY execution implemented, exact-head verification pending

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


## Feature 018 dependency-first constructor planning

`plan_elf32_init_array_calls` consumes one dependency graph and root object.
It performs a transient depth-first walk in stored dependency-edge order,
marking objects visiting before recursion and complete after contribution.
Re-entering a visiting object suppresses a cycle edge; reaching a complete
object suppresses a shared dependency duplicate. Every first visit consumes the
caller-selected `max_objects` budget.

After dependencies complete, the object's validated INIT_ARRAY descriptor is
decoded through feature 017 using the remaining total `max_entries` budget.
Raw sentinel entries still consume that budget. Values `0` and
`0xffffffff` are omitted from the final call list; every other entry retains
the defining object index, original array index, and raw 32-bit function value,
including bit 0 for later ARM/Thumb execution policy.

Invalid roots/edges, object-ceiling exhaustion, total-entry exhaustion, or
nested lifecycle decode failures return no successful partial call vector.
Planning is read-only: no guest byte, mapping, dependency edge, or persistent
constructor state is changed.

Feature 018 deliberately stops before legacy `DT_INIT`, PREINIT_ARRAY,
FINI_ARRAY/destructor planning, persisted constructor-called state, guest CPU
invocation, dlopen lifecycle, or unload.


## Feature 018 validation

At result revision `002a938ad0e5bd657716e1cc978d65e0ade4869e`:

- Linux A32 smoke check `108303835694` PASSed;
- Android x86_64 address-space probe check `108303835697` PASSed;
- Android arm64-v8a cross-build check `108303835668` PASSed.

This establishes the read-only dependency-first planning contract only. Guest
constructor invocation, persistent called-state, legacy DT_INIT/PREINIT,
FINI_ARRAY/destructor ordering, dlopen lifecycle, and unload remain separate.


## Feature 019 bounded constructor execution

The CPU seam now accepts an optional normalized stop PC. Execution checks that
PC before the first instruction and after each stepped instruction, stopping
before code fetch at the target. This makes an unmapped logical return target a
valid bounded-call terminator and removes the executable sentinel-loop
requirement from lifecycle execution. Existing callers that omit the stop PC
retain fixed-instruction behavior.

`execute_elf32_init_calls` consumes a feature-018 call plan without owning
guest mappings. The caller provides an 8-byte-aligned guest stack top, a
normalized return-stop PC, and a finite per-call instruction ceiling. For each
call the executor zeroes deterministic register state, derives ARM/Thumb from
function bit 0, restores SP, sets interworking LR, and runs through the generic
CPU seam.

Calls that reach the stop PC complete in order. Constructor guest-memory writes
are intentionally preserved for following calls. CPU exceptions, memory
faults, invalid function/options, or an exhausted instruction ceiling stop the
sequence and report completed-call count plus failing call/object provenance.
No later call is claimed or attempted.

The executor does not allocate a stack, map/protect/unmap memory, roll back
constructor side effects, persist constructor-called state, or implement
legacy INIT/PREINIT/destructor lifecycle.
