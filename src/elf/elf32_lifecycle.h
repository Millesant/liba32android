#pragma once

#include <cstdint>
#include <vector>

#include "elf/elf32_linker_metadata.h"
#include "memory/guest_memory.h"

namespace liba32android::elf {

struct Elf32FunctionArrayDecodeOptions {
    std::uint32_t max_entries{};
};

enum class Elf32FunctionArrayDecodeError : std::uint8_t {
    None = 0,
    InvalidArraySize,
    RangeOverflow,
    TooManyEntries,
    ReadFailed,
};

struct Elf32FunctionArrayDecodeResult {
    Elf32FunctionArrayDecodeError error{Elf32FunctionArrayDecodeError::None};
    std::vector<std::uint32_t> entries;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32FunctionArrayDecodeError::None;
    }
};

// Decode one validated ELF32 INIT_ARRAY/FINI_ARRAY descriptor without guest
// mutation. Entries are returned as raw logical 32-bit values in declaration
// order. Sentinel filtering and function execution are lifecycle-policy work
// outside this primitive.
[[nodiscard]] Elf32FunctionArrayDecodeResult decode_elf32_function_array(
    const memory::GuestMemory& memory,
    const Elf32FunctionArrayMetadata& array,
    const Elf32FunctionArrayDecodeOptions& options);

[[nodiscard]] const char* to_string(
    Elf32FunctionArrayDecodeError error) noexcept;

}  // namespace liba32android::elf
