# Design — ARM32 guest liblog write shim/provider

## Shared protocol definition

`src/compat/a32_android_log_shim.h` is deliberately valid in two modes:

- preprocessing from ARM assembly sees only
  `LIBA32ANDROID_A32_ANDROID_LOG_WRITE_SHIM_SVC`;
- C++ additionally sees the typed SVC constant, canonical SONAME/identity, and
  exact-name catalog-entry helper.

This prevents duplicated service-number literals between the generated guest
shim and host integration code.

## Guest ELF pair

The pinned-NDK builder creates two freestanding ARM-mode DSOs:

- `liblog.so` with SONAME `liblog.so`, exporting
  `__android_log_write` as `svc #0xa0; bx lr`;
- `liba32android_android_log_consumer.so`, exporting
  `fixture_android_log_write(tag,text)` and linking against the generated
  shim so the ELF contains `DT_NEEDED liblog.so` and a JUMP_SLOT import.

Both use `-nostdlib`, no build ID, explicit ARM mode, and 16 KiB maximum page
size. CI builds the pair twice and byte-compares each output.

## Provider/ownership

The host integration owns the generated shim bytes. The helper returns an
`Elf32DependencyCatalogEntry` whose string views are static and whose image
span borrows those bytes. `Elf32DependencyCatalogProvider` remains the only
provider implementation; feature 027 adds no parallel provider hierarchy.

## Integration execution

Load the consumer as object 0 with the catalog provider. Require object 1 to be
the canonical shim identity and the single dependency edge to target it.
Resolve the consumer function and `__android_log_write`, then apply the
consumer's combined relocation transaction and require the one write to be
`R_ARM_JUMP_SLOT` targeting the shim symbol.

Map separate bounded data and stack regions. Seed consumer r0/r1 with logical
tag/text guest addresses, set LR to an unmapped requested stop PC, and execute
through `execute_a32_with_services`.

The compiled consumer supplies priority 4 in r0 and forwards tag/text to r1/r2.
The relocated PLT reaches the real shim SVC, the exact registry selects the
feature-026 service, the sink returns 1, and guest execution resumes through
both returns to the stop PC.

## Failure/limits

All ELF/provider/relocation bounds reuse existing caller-selected limits. Guest
string limits remain feature-026 limits. The integration maps only its explicit
data/stack pages and never broadens loaded ELF permissions.

## Non-goals

No binary vendoring/embedding, automatic platform-catalog installation,
filesystem/namespace selection, host liblog dependency, log print/vprint
varargs, or broader platform API emulation.
