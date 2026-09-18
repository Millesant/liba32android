#pragma once

#include <cstdint>
#include <string>

#include "elf/elf32_linker_metadata.h"
#include "memory/guest_memory.h"

namespace liba32android::elf {

enum class Elf32LinkerStringError : std::uint8_t {
    None = 0,
    MissingStringTable,
    StringOffsetOutOfRange,
    AddressOverflow,
    ReadFailed,
    UnterminatedString,
    StringTooLong,
};

struct Elf32LinkerStringOptions {
    // Maximum number of non-NUL payload bytes that may be materialized.
    // The terminating NUL may appear immediately after this many bytes.
    std::uint32_t max_string_bytes{};
};

struct Elf32SingleStringResult {
    Elf32LinkerStringError error{Elf32LinkerStringError::None};
    std::string value;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32LinkerStringError::None;
    }
};

// Read one NUL-terminated byte string from a validated ELF32 string-table
// descriptor. The function is read-only, uses GuestMemory exclusively, and
// requires an explicit per-string payload ceiling.
[[nodiscard]] Elf32SingleStringResult read_elf32_string_table_entry(
    const memory::GuestMemory& memory,
    const Elf32StringTableMetadata& string_table,
    std::uint32_t offset,
    const Elf32LinkerStringOptions& options);

[[nodiscard]] const char* to_string(Elf32LinkerStringError error) noexcept;

}  // namespace liba32android::elf
