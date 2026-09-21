# Fedora x86_64 Android 16 KiB emulator environment — 2026-09-21

## Scope

Environment evidence plus the first standalone project-probe attempt. It does not claim AArch64 liba32android/Dynarmic runtime execution on this target.

## Observed

- Host: native Fedora 44, x86_64.
- CPU virtualization: VMX exposed; `/dev/kvm` present.
- Android Emulator: 37.1.11.0, build 15917651.
- AVD: `liba32_16k`.
- System image: Android API 35, `google_apis_ps16k/x86_64`.
- KVM acceleration check: installed and usable, version 12.
- ADB target: `emulator-5554`, state `device`.
- `getconf PAGE_SIZE`: `16384`.
- `uname -m`: `x86_64`.
- Android release: `15`.
- Android SDK: `35`.
- Kernel release: `6.6.50-android15-8-g8adecb593e9b-ab12525588`.
- A full boot completed after starting with host OpenGL and Vulkan disabled.

Successful launch command:

```sh
emulator -avd liba32_16k \
  -no-window \
  -no-audio \
  -no-boot-anim \
  -no-snapshot \
  -gpu host \
  -feature -Vulkan
```

## Failed setup path

The same emulator initially selected a software/Lavapipe renderer and terminated with a host segmentation fault before completing a cold boot. This is host-emulator behavior, not project runtime evidence.

## First project-probe attempt

The non-generated-code probe path executed far enough to record:

- `page_size=16384`, `arch=x86_64`, Android 15 / SDK 35;
- `fastmem_4g.status=reserved` at a high host address;
- `fastmem_4g.commit_page=success` and `fastmem_4g.unmap=success`;
- exact `MAP_FIXED_NOREPLACE` mappings at 0x10000, 0x100000, 0x1000000, 0x10000000, 0x40000000, and 0x80000000;
- collision checks returning `EEXIST` for every exact mapping;
- `jit_wx.mprotect_rw_to_rx=success`;
- `jit_wx.execute=NOT_RUN`;
- `probe.complete=true`.

The same harness invocation emitted many Android linker warnings about unknown dynamic entries and an `A32CRASH|component=android_address_space_probe|signal=11|...` marker. The harness did not emit `android_16k_probe_validation.status=PASS` and no generated-code return-42 markers were observed.

The source-commit identity was not included in the pasted device output. The run was performed after PR #29 merged, but this evidence does not independently prove the local checkout SHA.

## Failure classification

The project pins Android NDK r27d. The Android 16 KiB guidance says NDK r27 and lower require `-Wl,-z,max-page-size=16384` and `-Wl,-z,common-page-size=16384` for 16 KiB ELF alignment, and documents runtime SIGSEGV behavior from incompatible RELRO alignment. The project did not apply those flags to its final Android ELF targets before this run.

A focused fix is in progress on `fix/android-16k-elf-alignment`; validation remains pending.

Source: https://developer.android.com/guide/practices/page-sizes

## Not demonstrated

- generated-code return-42 execution on this 16 KiB target: NOT RUN / NOT OBSERVED;
- complete `run_android_16k_probe_validation.sh` PASS: FAIL for this attempt;
- AArch64 `liba32android.so` / Dynarmic behavior with 16 KiB pages: NOT RUN.
