# Android guest address-space research

Status: PARTIAL — probe cross-build TESTED/PASS, device evidence NOT RUN

## Scope

This note separates three questions that were previously easy to conflate:

1. Can an AArch64 Android process reserve a contiguous 4 GiB host range suitable for Dynarmic fastmem?
2. Can selected guest ranges be mapped at the same numeric low virtual addresses on the host without clobbering existing mappings?
3. Can the process create writable code pages and transition them to executable under a W^X policy suitable for a JIT?

A positive answer to (1) does **not** require a positive answer to (2). Guest-pointer identity and Dynarmic fastmem are independent strategies.

## Directly observed upstream behavior

### Dynarmic pinned by this project

At `azahar-emu/dynarmic` commit `e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`, `Dynarmic::A32::UserConfig` exposes three relevant memory paths:

- memory callbacks;
- a 1M-entry 4 KiB page table with callback fallback for null entries;
- `fastmem_pointer`, documented as the beginning of a 4 GiB host address space arranged like guest memory, with page-fault fallback/recompilation.

`fastmem_pointer` is a host `uintptr_t`; the API does not require that the host base be below 4 GiB or numerically equal to guest addresses.

Source: https://github.com/azahar-emu/dynarmic/blob/e77b1ba0b7da7cbe93021b01a663acfe7c4dd516/src/dynarmic/interface/A32/config.h

### `MAP_FIXED_NOREPLACE`

Linux documents `MAP_FIXED_NOREPLACE` as available since kernel 4.17. It requests an exact address without replacing an existing mapping and reports a collision as `EEXIST`.

Linux also documents an important compatibility behavior: older kernels can ignore the unknown flag and treat the address as a hint, so callers must verify that the returned pointer equals the requested address rather than treating any successful `mmap` as proof.

Source: https://man7.org/linux/man-pages/man2/mmap.2.html

Current bionic kernel UAPI headers define `MAP_FIXED_NOREPLACE` as `0x100000`. Header availability does not by itself prove that the running device kernel implements the semantics, so runtime probing remains required.

Source: https://android.googlesource.com/platform/bionic/+/master/libc/kernel/uapi/asm-generic/mman-common.h

Android devices span multiple kernel generations across platform releases and upgrade paths. Kernel version therefore belongs in every address-space evidence record rather than being inferred only from Android API level.

Source: https://source.android.com/docs/core/architecture/kernel/android-common

## Reference binary evidence: `libemu32.so`

Artifact SHA-256:

`a467c34bc1543a2a193191ad42c4ac3a8e4a00181e83fa223abf7d42bc421119`

Static inspection of the provided reference library directly observed:

- ELF64 little-endian AArch64 shared object;
- Android minimum platform note 21;
- built by NDK r27c (`12479018`);
- imports for `mmap`, `mprotect`, `munmap`, `sysconf`, `getenv`, `strtoul`/`strtoull`;
- strings `EMU32_ARENA_BASE`, `EMU32_ARENA_MB`, and `EMU32_ARENA_LOG`;
- strings associated with a translator/block cache such as `tabela de blocos cheia`, `instrucao invalida`, and `instrucao sem traducao`.

**Observed:** the binary has configurable arena-related environment strings and uses the standard memory-mapping APIs above.

**Inference:** it likely has a configurable guest/JIT arena whose placement or size can be influenced at runtime.

**Not proven:** exact arena layout, use of `MAP_FIXED`, use of `MAP_FIXED_NOREPLACE`, a 4 GiB reservation, guest-pointer identity, or Dynarmic-style fastmem. Those require xrefs/disassembly or runtime evidence and must not be assumed from strings alone.

## Project decision boundary

The runtime keeps guest virtual addresses as 32-bit logical values. The memory layer owns translation to host pointers.

Candidate implementations remain tiered:

1. **Callbacks** — correctness baseline and fallback; already integrated.
2. **Page table** — potential acceleration for mapped pages without requiring one contiguous 4 GiB reservation.
3. **Fastmem** — optional acceleration when a contiguous 4 GiB host reservation succeeds.
4. **Direct low-VA identity** — optional experimental optimization only when device evidence proves the required guest ranges can be mapped safely at matching host addresses.

No loader, ABI bridge, or compatibility library may rely on guest pointer == host pointer unless a future accepted decision explicitly changes this rule.

## Android probe

`android_address_space_probe` is a standalone arm64-v8a executable. It intentionally does not link the runtime so that OS address-space behavior can be measured independently of Dynarmic.

It records:

- Android API level used at compile time;
- kernel release and page size;
- `/proc/sys/vm/mmap_min_addr` when readable;
- mappings intersecting the low 4 GiB, without filesystem paths;
- whether an anonymous `PROT_NONE` 4 GiB reservation succeeds and whether one page inside it can be committed RW;
- exact low-address `MAP_FIXED_NOREPLACE` attempts at several representative addresses, including collision semantics;
- whether an anonymous RW page can transition to RX;
- optionally, whether generated AArch64 code executes and returns 42.

The probe never uses destructive `MAP_FIXED` against unowned address ranges.

### GitHub CI evidence

GitHub Actions run `35012394383`, PR #3 implementation commit `11301da46195e323cc8cf56eee5ae9fd60ec1a1c`:

- existing Linux build and CTest: PASS, 8/8 tests, 0 failures;
- Android `arm64-v8a` runtime configure/build/link: PASS;
- `android_address_space_probe` compile/link with NDK `27.3.13750724`, API 26: PASS;
- artifact upload: PASS;
- artifact ID: `10413174866`;
- artifact size: 19,816 bytes;
- artifact digest: `sha256:e1b91c649fc1c448c0ea00455d00c68ce62b0c83eca7aaa10cc1765f41a5e894`.

This is build evidence only. No Android `mmap`, `mprotect`, fastmem reservation or generated-code result is inferred from CI.

### Device execution

Example once a device/emulator path is available:

```sh
adb push android_address_space_probe /data/local/tmp/
adb shell chmod 755 /data/local/tmp/android_address_space_probe
adb shell /data/local/tmp/android_address_space_probe
adb shell /data/local/tmp/android_address_space_probe --execute-generated-code
```

The first invocation measures mapping policy without executing generated code. The second additionally tests the RW->RX generated-code path and should report `jit_wx.result_check=PASS` only if it actually returns 42.

Store device outputs with Android build/API, kernel release, device architecture and probe commit. Do not generalize one device result to all Android versions.

## Current recommendation

Keep callbacks as the correctness baseline. Do not select fastmem or direct low-VA identity until device evidence exists. If 4 GiB reservation proves reliable while low-VA exact mappings are unreliable, fastmem at an arbitrary high host base remains viable because the two properties are independent.
