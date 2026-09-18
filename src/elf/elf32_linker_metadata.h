#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "elf/elf32_dynamic.h"

namespace liba32android::elf {

enum class Elf32LinkerMetadataError : std::uint8_t {
    None = 0,
    DuplicateSingleton,
    IncompleteStringTable,
    IncompleteSymbolTable,
    IncompleteRelTable,
};

struct Elf32DynamicStringTableMetadata {
    // Raw DT_STRTAB value. T002 will rebase and validate this as a guest VA.
    std::uint32_t address_value{};
    std::uint32_t size{};
};

struct Elf32DynamicSymbolTableMetadata {
    // Raw DT_SYMTAB value. T002 will rebase and validate this as a guest VA.
    std::uint32_t address_value{};
    std::uint32_t entry_size{};
};

struct Elf32DynamicRelTableMetadata {
    // Raw DT_REL value. T002 will rebase and validate this as a guest VA.
    std::uint32_t address_value{};
    std::uint32_t size{};
    std::uint32_t entry_size{};
};

struct Elf32LinkerMetadata {
    std::optional<Elf32DynamicStringTableMetadata> string_table;
    std::optional<Elf32DynamicSymbolTableMetadata> symbol_table;
    std::optional<Elf32DynamicRelTableMetadata> rel_table;
    std::optional<std::uint32_t> soname_offset;
    std::vector<std::uint32_t> needed_offsets;
};

struct Elf32LinkerMetadataResult {
    Elf32LinkerMetadataError error{Elf32LinkerMetadataError::None};
    Elf32LinkerMetadata metadata;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32LinkerMetadataError::None;
    }
};

// Collect the first supported dynamic-linker tag set from structurally parsed
// Elf32_Dyn entries. Pointer-like values remain raw in this T001 layer; checked
// load-bias rebasing and GuestMemory range validation are added by T002.
[[nodiscard]] Elf32LinkerMetadataResult collect_elf32_linker_metadata(
    std::span<const Elf32DynamicEntry> entries);

[[nodiscard]] const char* to_string(Elf32LinkerMetadataError error) noexcept;

}  // namespace liba32android::elf
