# Design — ELF32 Linker Strings

Status: ready for implementation

## Context

`001-elf32-linker-metadata` now validates STRTAB/STRSZ and records SONAME plus ordered `DT_NEEDED` offsets, but deliberately leaves those values as offsets. It also validates the declared STRTAB guest range as readable.

This feature consumes that contract and performs the next bounded semantic step: locating each referenced NUL-terminated byte string inside STRTAB and copying only the requested names into host-owned result strings.

## Chosen Approach

Add a separate engine-independent component:

`src/elf/elf32_linker_strings.{h,cpp}`

Conceptual API:

```cpp
struct Elf32LinkerStringOptions {
    std::uint32_t max_string_bytes{};
};

Elf32SingleStringResult read_elf32_string_table_entry(
    const memory::GuestMemory& memory,
    const Elf32StringTableMetadata& string_table,
    std::uint32_t offset,
    const Elf32LinkerStringOptions& options);

struct Elf32LinkerStrings {
    std::optional<std::string> soname;
    std::vector<std::string> needed;
};

Elf32LinkerStringResult build_elf32_linker_strings(
    const memory::GuestMemory& memory,
    const Elf32LinkerMetadata& metadata,
    const Elf32LinkerStringOptions& options);
```

The single-entry reader is deliberately exposed within the internal ELF API so T001 is independently testable and later symbol-name work can reuse the same bounded STRTAB primitive. The aggregate SONAME/NEEDED builder is layered on top in T002.

There is intentionally no implicit default for `max_string_bytes` in the first slice. Callers must choose an explicit finite payload limit.

Exact public type names may vary, but the semantics must remain equivalent.

## Architecture / Data Flow

```text
elf32_loader
    |
    v
elf32_dynamic
    | raw d_tag / d_val
    v
elf32_linker_metadata
    | validated STRTAB guest range
    | SONAME offset
    | ordered NEEDED offsets
    v
elf32_linker_strings
    | materialized SONAME / NEEDED byte strings
    v
future dependency loader
```

The new layer does not know search paths, filesystem state, link maps, namespaces, symbol tables, relocations, Dynarmic, or application profiles.

## Interfaces / Contracts

### Input metadata

The consumer accepts `Elf32LinkerMetadata` rather than raw dynamic entries.

Only these fields are relevant:

- `string_table.guest_address`;
- `string_table.size`;
- `soname_offset`;
- ordered `needed_offsets`.

The consumer remains defensive because `Elf32LinkerMetadata` is a constructible C++ struct rather than an opaque proof token. It rechecks string-table presence and offset bounds before reading.

### Output

Successful output owns copied byte strings:

- optional SONAME;
- ordered vector of NEEDED names.

No guest pointer or string-table view escapes through the result.

`std::string` is used as a byte container. No UTF-8 assumption is made.

## Bounded String Reader

Implement one narrow reusable helper, `read_elf32_string_table_entry`, that reads a single offset from the validated STRTAB descriptor.

Inputs:

- `GuestMemory`;
- STRTAB guest base + size;
- offset;
- explicit maximum payload bytes.

Algorithm:

1. reject `offset >= strtab.size`;
2. compute `guest_address = strtab.guest_address + offset` with 64-bit checked arithmetic;
3. compute `remaining = strtab.size - offset`;
4. scan through guest memory in a small fixed-size stack buffer;
5. inspect at most enough bytes to decide whether:
   - a NUL occurs with payload length `<= max_string_bytes`;
   - payload length has exceeded `max_string_bytes`;
   - STRTAB ends without NUL;
6. on success, materialize exactly the payload bytes before NUL.

The scan must never allocate a buffer proportional to `remaining`.

## Limit / Unterminated Precedence

The error classification must be deterministic.

Let `max` be the allowed payload-byte count.

- NUL at payload position `0..max`: success.
- No NUL and STRTAB ends after at most `max` payload bytes: `UnterminatedString`.
- No NUL after observing `max + 1` payload bytes while STRTAB still provides those bytes: `StringTooLong`.

This lets a payload exactly equal to the ceiling succeed if immediately followed by NUL.

Use 64-bit counters so the `max + 1` decision itself cannot overflow 32-bit arithmetic.

## Read Strategy

Use a fixed-size chunk, tentatively 256 bytes, matching the bounded-read style already used by linker metadata.

Reads may be shorter at:

- the end of STRTAB;
- the maximum-length decision boundary.

Although linker metadata already validated STRTAB readability, string reads must still propagate `GuestMemory::read` failure because memory state/permissions could change between layers and manually constructed metadata is possible.

## Address Arithmetic

Never compute a guest pointer through native pointer arithmetic.

Every guest address calculation uses 64-bit intermediates and rejects values above `UINT32_MAX` before narrowing.

The declared STRTAB range may already be validated by the preceding layer, but the string consumer remains safe when handed a manually constructed descriptor.

## Aggregate Semantics

The aggregate builder is all-or-nothing:

1. validate required STRTAB presence when SONAME/NEEDED exists;
2. read SONAME if present;
3. read NEEDED names in stored order;
4. return the complete result only if all requested strings succeed.

Repeated offsets are read/preserved as repeated names. No deduplication occurs.

A metadata object with no SONAME and no NEEDED entries succeeds with an empty result and does not require STRTAB.

## Byte Semantics

No encoding validation or normalization occurs.

Examples that remain valid at this layer:

- empty string;
- ordinary ASCII library name;
- arbitrary non-zero byte sequences followed by NUL, including invalid UTF-8.

The future dependency-loader feature may impose stronger filename/path policy, but this slice must not silently embed it.

## Failure Semantics

Use an explicit result/error model consistent with existing ELF components.

Required categories:

- `MissingStringTable`;
- `StringOffsetOutOfRange`;
- `AddressOverflow`;
- `ReadFailed`;
- `UnterminatedString`;
- `StringTooLong`.

On failure, the returned result is unsuccessful and must not expose a successful partial string set.

## Security / Compatibility

- Caller-selected resource ceiling is mandatory.
- No filesystem access occurs.
- No host pointer identity assumption occurs.
- Guest memory is read-only through `GuestMemory`.
- Empty and non-UTF-8 strings are not rejected at this structural/semantic boundary.
- Repeated NEEDED names are preserved for later linker policy rather than normalized away.

## Test Strategy

### Synthetic tests

Add focused tests for:

- SONAME + multiple NEEDED names;
- repeated NEEDED offset/name order;
- no strings and no STRTAB;
- empty string;
- arbitrary non-UTF-8 bytes;
- missing STRTAB with requested name;
- offset equal to STRSZ;
- address overflow from a manually constructed descriptor;
- GuestMemory read failure;
- NUL before STRTAB end;
- unterminated-at-table-end behavior;
- payload exactly at maximum + NUL;
- payload exceeding maximum;
- failure in a later NEEDED entry produces no successful overall result;
- guest bytes remain unchanged.

### Real fixture

Extend/add a fixture integration test that:

1. loads the pinned NDK-generated ARM32 fixture;
2. parses its PT_DYNAMIC;
3. builds validated linker metadata;
4. consumes linker strings with an explicit ceiling large enough for the known fixture name;
5. requires SONAME exactly `liba32android_loader_fixture.so`;
6. requires zero NEEDED names.

The current fixture cannot prove repeated NEEDED behavior because its freestanding build intentionally has no dependencies.

### CI

GitHub Actions remains the authoritative build/test environment.

Completion requires:

- new synthetic string tests PASS;
- real fixture string integration PASS;
- existing loader/dynamic/linker-metadata tests PASS;
- exact `liba32android.so` output-name check PASS;
- Android `arm64-v8a` runtime/diagnostics cross-build PASS.

No Termux run is required for this host-observable, read-only metadata/string slice.

## Performance Strategy

Read only strings actually referenced by SONAME/NEEDED offsets.

Use bounded stack buffers and avoid copying the whole STRTAB. Complexity is proportional to the requested name bytes scanned, bounded per string by caller policy.

## Alternatives Considered

### Read names inside `elf32_linker_metadata`

Rejected. The previous layer intentionally validates descriptors/offsets without materializing string content. Keeping string consumption separate preserves a narrow failure boundary.

### Pass raw `Elf32DynamicEntry` values again

Rejected. That would duplicate tag interpretation and bypass the validated STRTAB/offset contract established by `001`.

### Return guest string views instead of copied strings

Deferred. `GuestMemory` does not promise a stable contiguous host view, and exposing host pointers would violate the guest-address invariant. Copying only requested bounded strings is simpler and safe across memory backends.

### Impose a fixed global maximum string length

Rejected for this slice. ELF itself does not provide the project with a repository-established runtime-wide limit. Requiring an explicit caller-supplied ceiling keeps resource usage bounded without silently creating a universal compatibility policy.

### Reject empty or non-UTF-8 names here

Rejected. Those are higher-level dependency/path policy decisions, not required for safe bounded STRTAB consumption.

## Assumptions

- Linker metadata was normally produced by `build_elf32_linker_metadata`, but the consumer still validates the fields it directly relies upon.
- The caller can choose a suitable finite maximum string length for its use case.
- Dependency loading and pathname policy will consume the materialized names in a later feature package.
