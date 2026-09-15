# Decisions

## D-0001 — Use Dynarmic behind an internal CPU adapter

Status: Accepted
Date: 2026-09-15

### Context

The runtime needs AArch32 ARM/Thumb execution inside an eventual AArch64 Android process. Rebuilding a decoder, IR, optimizer, AArch64 emitter and code cache is unnecessary if a maintained embeddable engine satisfies the CPU requirements.

### Decision

Use Dynarmic for M0, pinned to `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`, and isolate all Dynarmic-specific types under `src/cpu/`.

### Rationale

The selected Dynarmic fork directly exposes A32 guest support, AArch64 host support, Android support, memory callbacks/fastmem/page-table mechanisms, cache invalidation and a C++ embedding API. Dynarmic itself is 0BSD licensed. QEMU and Unicorn remain useful references but impose a larger/QEMU-derived integration and GPLv2 licensing surface; FEX and Box64 target x86-family guests rather than AArch32.

### Consequences

- CPU execution can advance independently from ELF and Android compatibility layers.
- Dynarmic must not leak into higher-level runtime APIs.
- Engine inaccuracies documented upstream require targeted regression tests and reference comparisons.
- Dependency commit and license must remain auditable.

### Supersedes / Superseded by

None.

## D-0002 — Put a generic guest-memory contract between the CPU adapter and address-space implementation

Status: Accepted
Date: 2026-09-15

### Context

M0 embedded a fixed 4 KiB byte array directly inside the Dynarmic callback implementation. That was sufficient for a one-instruction smoke test but coupled CPU execution to a test-only memory model and could not grow into ELF loading or a real guest address space cleanly.

### Decision

Introduce `memory::GuestMemory` as the engine-independent read/write contract consumed by the CPU adapter. Use `LinearGuestMemory` only as the first bounded contiguous implementation for tests and early runtime work. Surface callback access failures as an execution `memory_fault`.

Do not select low-VA direct mapping, Dynarmic page tables or fastmem yet.

### Rationale

This keeps the CPU engine unaware of mapping policy while allowing memory implementations to evolve independently. It also makes the current correctness behavior testable without prematurely committing to Android-specific virtual-address assumptions.

### Consequences

- Dynarmic callbacks translate through `GuestMemory`; higher layers do not depend on Dynarmic callback types.
- ELF loading can later target the guest-address-space layer rather than CPU internals.
- `LinearGuestMemory` is not evidence that a contiguous 4 GiB host reservation is feasible on Android.
- Mapping permissions, regions, guard pages, direct mapping and fastmem remain separate M2 decisions requiring evidence.

### Supersedes / Superseded by

None.

## D-0003 — Keep guest virtual addresses independent from host pointer identity

Status: Accepted
Date: 2026-09-15

### Context

The reference binary exposes arena-related configuration strings, and Dynarmic offers both page-table and 4 GiB fastmem mechanisms. It would be easy to conflate a contiguous fastmem reservation with the stronger assumption that every 32-bit guest pointer must equal the numeric host virtual address.

### Decision

Treat AArch32 guest addresses as logical 32-bit virtual addresses owned by the guest-memory layer. Do not require guest pointer == host pointer in the generic runtime.

Treat these as separate optional implementation strategies:

- callbacks as the correctness fallback;
- page-table acceleration for mapped guest pages;
- fastmem when a contiguous 4 GiB host reservation is available;
- direct low-VA pointer identity only as an experimental optimization backed by device evidence.

### Rationale

The pinned Dynarmic API accepts an arbitrary host `uintptr_t` as the beginning of its 4 GiB fastmem space; low-VA identity is not required by that mechanism. Keeping translation explicit also prevents ELF, ABI and bridge code from depending on an Android virtual-address assumption that has not been proven across devices.

### Consequences

- Guest pointers stored in registers/memory remain 32-bit guest values, not raw host pointers.
- Loader and ABI layers must use the guest-memory/address translation boundary.
- A successful high-address 4 GiB reservation may enable fastmem even if exact low-VA mappings are unavailable.
- Direct low-VA mapping requires a future explicit decision and measured compatibility evidence.

### Supersedes / Superseded by

None.
