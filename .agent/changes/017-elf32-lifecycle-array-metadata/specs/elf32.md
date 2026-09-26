# ELF32 spec delta — feature 017

## L32-E004 extension

Validated linker metadata additionally retains paired INIT_ARRAY/INIT_ARRAYSZ
and FINI_ARRAY/FINI_ARRAYSZ guest descriptors. Each address is rebased exactly
once, each byte size is divisible by four, and each non-empty declared range is
readable.

## New lifecycle-array contract

A caller-bounded read-only decoder materializes raw ELF32 function-array
entries in declaration order. It preserves null/all-ones sentinels and fails
explicitly for non-integral size, range overflow, entry-ceiling excess, or read
failure. It performs no guest mutation or function execution.

## Deferred behavior

DT_INIT/DT_FINI, PREINIT_ARRAY, dependency-order lifecycle planning, sentinel
filtering, recursion guards, constructor/destructor execution, dlopen lifecycle,
and unload remain outside this change.
