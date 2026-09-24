# Design — ELF32 GNU RELRO protection

Status: ACTIVE — T001 metadata implementation prepared; exact-head validation NOT RUN

## Context

The runtime now has a complete eager relocation path for the currently supported main and PLT REL forms. GNU RELRO is the next post-relocation hardening step: selected writable data pages are needed while relocations run, then become read-only.

The existing loader already owns program-header structural validation, and `MappedGuestMemory` owns page permissions. Feature 010 connects those layers without moving relocation semantics into the loader.

## T001 load metadata

Extend the load plan with:

```cpp
struct Elf32LoadPlanRelroSegment {
    std::uint32_t virtual_address{};
    std::uint32_t memory_size{};
    std::uint32_t mapping_start{};
    std::uint64_t mapping_end{};
};
```

and `std::vector<Elf32LoadPlanRelroSegment> relro_segments`.

Recognize `PT_GNU_RELRO` while reading the program-header table. Store exact `p_vaddr/p_memsz`, derive the current-host-page rounded protection range, then validate after all `PT_LOAD` entries are known.

Validation walks every rounded page and requires coverage by a readable non-empty load mapping. This intentionally validates the mprotect range rather than only the exact byte range.

The loader result adds:

```cpp
struct Elf32RelroSegment {
    std::uint32_t guest_address{};
    std::uint32_t memory_size{};
    std::uint32_t mapping_start{};
    std::uint64_t mapping_size{};
};
```

The values are load-biased logical guest addresses. Mapping permissions are not changed by T001.

## T002 sealing API

Add `src/elf/elf32_relro.{h,cpp}`:

```cpp
struct Elf32RelroOptions {
    std::uint32_t max_pages{};
};

Elf32RelroSealResult seal_elf32_gnu_relro(
    memory::MappedGuestMemory& memory,
    const Elf32LoadResult& load,
    const Elf32RelroOptions& options);
```

The API operates on loader-owned metadata rather than reparsing the ELF image or dynamic array.

### Preflight

For every descriptor:

1. validate page-aligned/page-multiple mapped metadata;
2. validate exact guest range containment;
3. account page occurrences against `max_pages`;
4. enumerate pages;
5. sort/deduplicate page addresses;
6. require every page mapped;
7. accept only R or RW current permission shapes; reject unreadable or executable pages;
8. capture original permissions.

No permission changes occur before the full set passes.

### Commit/rollback

For each unique page in ascending order:

- already-R: no mutation needed;
- RW: call `protect(page, page_size, Read)`.

If protection fails later, restore previously changed pages in reverse order. Return a distinct rollback error if restoration cannot be established.

This is intentionally page-granular. The caller ceiling makes syscall/vector work bounded, and page-level records make rollback unambiguous even when RELRO headers overlap.

## Ordering

The loader never seals automatically.

The intended eager flow is:

```text
load object graph
 -> parse/link metadata
 -> resolve/apply main REL
 -> resolve/apply eager PLT REL
 -> seal GNU RELRO
 -> later runtime work
```

A future lazy-binding feature must revisit GOT/RELRO interaction explicitly; feature 010 must not weaken RELRO to reserve space for an unimplemented resolver.

## Real fixture validation

The existing pinned ARMv7 loader fixture is reused.

T001 extends the loader integration to parse the raw GNU RELRO program header independently and compare it with `Elf32LoadResult::relro_segments`.

T003 loads the same fixture through `load_elf32_dependency_graph`, applies the existing real main relocations, then seals RELRO. It snapshots bytes and page permissions so the test can prove:

- relocation target values survive sealing;
- all RELRO pages are R;
- writes into RELRO fail;
- pages outside RELRO retain their previous permissions;
- no guest code executes.

No new opaque binary fixture is required.

## Failure semantics

T001 extends `Elf32LoadError` with precise GNU RELRO structural errors.

T002 uses a separate RELRO error enum so mapping/metadata/preflight/protect/rollback failures are not conflated with loader or relocation errors.

Failure before the commit phase leaves permissions unchanged. Rollback failure is the only case where a partially sealed object may remain.

## Documentation obligations

At convergence update:

- `docs/architecture/elf32-loader.md` for RELRO metadata;
- a focused RELRO architecture section/document;
- `README.md`;
- `.agent/CONTEXT.md`, `.agent/STATE.md`, `.agent/NEXT.md`;
- feature change/spec/task/evidence records.

## Alternatives

### Seal RELRO inside load_elf32

Rejected. RELRO must remain writable until relocations complete.

### Reparse program headers during sealing

Rejected. The shared load plan/loader already owns structural ELF validation; downstream hardening should consume validated guest-only metadata.

### Add lazy binding first

Rejected. The runtime currently uses eager JUMP_SLOT and has no resolver lifetime/concurrency protocol. RELRO can be made correct for the current eager model without inventing that larger contract.

### Ignore host-page rounding

Rejected. `mprotect` is page-granular and Android bionic explicitly rounds the GNU RELRO range.

## Readiness

- The existing real fixture already contains GNU RELRO.
- Page protection is available through `MappedGuestMemory`.
- Eager relocation prerequisites are complete.
- The loader and post-relocation sealing responsibilities are separable.
- Acceptance/failure behavior is observable and bounded.
- T001-T004 are acyclic.
