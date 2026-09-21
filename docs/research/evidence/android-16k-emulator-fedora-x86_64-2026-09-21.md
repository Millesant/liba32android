# Fedora x86_64 Android 16 KiB emulator environment — 2026-09-21

## Scope

Environment evidence only. This record does not claim that the project address-space probe or the AArch64 liba32android/Dynarmic runtime executed on this target.

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

## Not demonstrated

- `android_address_space_probe` execution on this target: NOT RUN.
- 4 GiB reservation/commit behavior on this target: NOT RUN.
- `MAP_FIXED_NOREPLACE` behavior on this target: NOT RUN.
- RW -> RX and generated-code execution through the project probe: NOT RUN.
- AArch64 `liba32android.so` / Dynarmic behavior with 16 KiB pages: NOT RUN.
