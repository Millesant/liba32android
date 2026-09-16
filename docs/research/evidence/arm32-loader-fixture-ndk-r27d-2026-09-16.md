# Real ARM32 ELF loader fixture evidence — 2026-09-16

Status: TESTED / PASS on Linux host loader integration; real 16 KiB Android host execution NOT RUN

## Reproduction

The fixture is generated from source rather than stored as an opaque binary:

- source: `tests/fixtures/arm32_loader_fixture.c`
- build script: `tools/build_arm32_loader_fixture.sh`
- Android NDK: `27.3.13750724` (r27d)
- target compiler: `armv7a-linux-androideabi26-clang`
- target API: 26
- linker max page size: 16384 bytes

CI builds the fixture twice with the same pinned toolchain and compares the outputs byte-for-byte before running the loader integration test.

Observed fixture SHA-256 in GitHub Actions run `35109819830` (#58):

`6c2dbda2dec94eaa022ad09391ed3988c3a41828e5c1c065fc0101e5725b84c2`

Both independent builds produced that same digest and `cmp` succeeded.

## Observed ELF identity

`file` / `readelf` reported:

- ELF32
- little-endian
- type: `ET_DYN`
- machine: ARM
- EABI5
- entry point: `0x0`
- 9 program headers

The fixture contains exported code, initialized data and BSS and is built without libc/runtime dependencies.

## Observed PT_LOAD layout

| # | Offset | Virtual address | File size | Memory size | Flags | p_align |
|---|---:|---:|---:|---:|---|---:|
| 0 | `0x000000` | `0x00000000` | `0x224` | `0x224` | R | `0x4000` |
| 1 | `0x000224` | `0x00004224` | `0x48` | `0x48` | R E | `0x4000` |
| 2 | `0x00026c` | `0x0000826c` | `0x68` | `0x0d94` | RW | `0x4000` |
| 3 | `0x0002d4` | `0x0000c2d4` | `0x4` | `0x8` | RW | `0x4000` |

Additional observed program headers include `PT_DYNAMIC`, `GNU_RELRO`, `GNU_STACK`, and ARM `EXIDX`.

Observed `PT_DYNAMIC`:

- file offset: `0x26c`
- virtual address: `0x826c`
- file/memory size: `0x60`

Observed dynamic tags include SONAME, `DT_REL`, `DT_RELSZ`, `DT_RELENT`, `DT_SYMTAB`, `DT_SYMENT`, `DT_STRTAB`, `DT_STRSZ`, GNU hash, and bind-now flags. No `DT_NEEDED` entry was shown for this freestanding fixture.

## Observed loader result

GitHub Actions run `35109819830` (#58), current PR implementation checkpoint:

- fixture generation: PASS
- repeated byte-identical generation: PASS
- Linux build: PASS
- CTest: 17/17 PASS
- `elf32_real_arm32_fixture`: PASS
- Android arm64 runtime/diagnostics cross-build: PASS
- loader evidence artifact upload: PASS

The loader-only integration test observed:

- `fixture.kind=android_armv7_et_dyn`
- `fixture.size=2024`
- `fixture.pt_load.count=4`
- `fixture.pt_dynamic=true`
- `fixture.has_bss=true`
- `fixture.max_p_align=16384`
- `fixture.host_page_size=4096`
- `fixture.misaligned_bias_rejected=true`
- `fixture.load_bias=0x2000000`
- `fixture.status=PASS`

For every PT_LOAD, the test compares the loader result to the real program header, verifies copied file bytes, verifies BSS zero-fill where `p_memsz > p_filesz`, and verifies final guest page permissions.

## Alignment finding

Observed evidence from the real fixture exposed a loader requirement that the synthetic fixtures did not force: ET_DYN load bias must preserve each PT_LOAD `p_align`, not only the current host page size.

The loader now rejects a host-page-aligned ET_DYN base when the resulting load bias violates the fixture's 16 KiB `p_align`. The real fixture test demonstrated that rejection with `fixture.misaligned_bias_rejected=true`.

## Evidence classification

### Observed

- The pinned NDK generated the same fixture bytes twice in CI.
- The generated file is ARM ELF32 ET_DYN.
- It has four PT_LOAD segments, all with `p_align=0x4000`.
- It contains PT_DYNAMIC, executable content, writable content and BSS.
- The current loader mapped the real fixture correctly on a 4096-byte-page Linux host.
- Exact PT_LOAD file bytes, BSS zero-fill and final permissions matched the fixture headers.
- An ET_DYN base whose load bias was 4 KiB-aligned but not 16 KiB-aligned was rejected.

### Inferred

From the observed PT_LOAD addresses/sizes, the four segments occupy distinct 16 KiB windows as well as distinct 4 KiB host-page ranges. Therefore this particular fixture does not demonstrate a need for shared-page PT_LOAD handling at 16 KiB granularity.

This is an inference from the program headers, not execution on a 16 KiB host.

### Not demonstrated

- `MappedGuestMemory` behavior on an actual 16 KiB Android kernel/page configuration.
- Loading this fixture through the runtime on a real Android device.
- Applying its dynamic relocations.
- Resolving its dynamic symbols/string table.
- GNU RELRO enforcement.
- ARM32 function execution from the loaded fixture.
- Broad Android ELF compatibility.

## Artifact

Run #58 uploaded `arm32-loader-fixture-b86149efcced4e9cfee2b70293d4e0279234a26e` containing:

- `liba32android_loader_fixture.so`
- `loader-evidence.txt`
- `program-headers.txt`

Artifact ID: `10452280814`
Artifact ZIP SHA-256: `45011484cca2274066b96389979c68261339ffebdb6b579a44692af10fd19e56`
