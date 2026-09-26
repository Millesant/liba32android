# A32 host-service registry

Status: feature 025 DONE; exact-head implementation CI PASSed at `c2fb94e450619378bb0b77880fb0454e53ce38b3`

## Boundary

`src/runtime/a32_service_registry.*` is a game-agnostic composition layer above
feature 024's `A32HostServiceHandler` interface. It maps exact A32 SVC
immediates to caller-owned child handlers without interpreting guest ABI
arguments or adding platform behavior.

The registry borrows a finite
`std::span<const A32HostServiceRegistryEntry>`. Every entry contains an exact
SVC immediate and a borrowed `A32HostServiceHandler*`. The entry array and
handler objects must outlive the registry.

## Lookup semantics

For each trapped SVC:

1. scan only the supplied finite entry span;
2. no matching entry -> `Unhandled`;
3. exactly one matching non-null handler -> invoke it with the original
   `GuestMemory`, exact immediate, mutable registers, and mutable CPSR;
4. a matching null handler, duplicate matching immediate, or direct self-entry
   -> `Failed` before any child handler is invoked.

Malformed entries for other immediates do not poison an otherwise exact lookup.
The registry performs no allocation and does not own or reorder handlers.

The child handler's `Handled`, `Unhandled`, or `Failed` result is returned
unchanged. Register, CPSR, and guest-memory effects performed by an invoked
child follow feature 024's existing side-effect semantics.

## Layering

The registry contains no Dynarmic types and no Android API constants. It does
not decode AAPCS arguments, read guest strings, generate or select ELF shims,
resolve symbols, model syscalls, or choose namespace/search policy.

Those behaviors remain separate compatibility/platform layers. In particular,
this feature establishes only a deterministic service-number composition seam
for later VLC/FMOD-oriented compatibility shims.

## Validation

The feature-025 runtime regression covers unknown IDs, exact child selection,
child result/state forwarding, duplicate IDs, matching null handlers, and
unrelated malformed entries. It also executes an ARM `svc #0x42; bx lr`
program through feature 024 with the registry as the dispatcher handler.

Exact-head implementation revision
`c2fb94e450619378bb0b77880fb0454e53ce38b3` PASSed:

- Linux A32 smoke check `108385436541`;
- Android x86_64 address-space probe check `108385436452`;
- Android arm64-v8a cross-build check `108385436554`.

These checks validate the generic registry and its registered regression; they
do not establish any Android API implementation, shim ELF, or application
compatibility.
