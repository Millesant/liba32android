#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "elf/elf32_dynamic.h"
#include "memory/guest_memory.h"

namespace liba32android::elf {

enum class Elf32LinkerMetadataError : std::uint8_t {
    None = 0,
    DuplicateSingleton,
    IncompleteStringTable,
    IncompleteSymbolTable,
    IncompleteRelTable,
    AddressOverflow,
    RangeOverflow,
    ReadFailed,
    InvalidSymbolEntrySize,
    InvalidRelEntrySize,
    InvalidRelSize,
    StringOffsetOutOfRange,
};

struct Elf32CollectedStringTableMetadata {
    std::uint32_t address_value{};
    std::uint32_t size{};
};

struct Elf32CollectedSymbolTableMetadata {
    std::uint32_t address_value{};
    std::uint32_t entry_size{};
};

struct Elf32CollectedHashTableMetadata {
    std::uint32_t address_value{};
};

struct Elf32CollectedRelTableMetadata {
    std::uint32_t address_value{};
    std::uint32_t size{};
    std::uint32_t entry_size{};
};

struct Elf32CollectedLinkerMetadata {
    std::optional<Elf32CollectedStringTableMetadata> string_table;
    std::optional<Elf32CollectedSymbolTableMetadata> symbol_table;
    std::optional<Elf32CollectedHashTableMetadata> sysv_hash_table;
    std::optional<Elf32CollectedHashTableMetadata> gnu_hash_table;
    std::optional<Elf32CollectedRelTableMetadata> rel_table;
    std::optional<std::uint32_t> soname_offset;
    std::vector<std::uint32_t> needed_offsets;
    bool has_symbol_versioning{};
};

struct Elf32CollectedLinkerMetadataResult {
    Elf32LinkerMetadataError error{Elf32LinkerMetadataError::None};
    Elf32CollectedLinkerMetadata metadata;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32LinkerMetadataError::None;
    }
};

struct Elf32StringTableMetadata {
    std::uint32_t guest_address{};
    std::uint32_t size{};
};

struct Elf32SymbolTableMetadata {
    std::uint32_t guest_address{};
    std::uint32_t entry_size{};
};

// Hash descriptors intentionally validate/rebase only the fixed table header.
// Variable-sized buckets/chains are bounded and interpreted by
// elf32_symbol_lookup, where caller-selected resource ceilings are available.
struct Elf32HashTableMetadata {
    std::uint32_t guest_address{};
};

struct Elf32RelTableMetadata {
    std::uint32_t guest_address{};
    std::uint32_t size{};
    std::uint32_t entry_size{};
};

struct Elf32LinkerMetadata {
    std::optional<Elf32StringTableMetadata> string_table;
    std::optional<Elf32SymbolTableMetadata> symbol_table;
    std::optional<Elf32HashTableMetadata> sysv_hash_table;
    std::optional<Elf32HashTableMetadata> gnu_hash_table;
    std::optional<Elf32RelTableMetadata> rel_table;
    std::optional<std::uint32_t> soname_offset;
    std::vector<std::uint32_t> needed_offsets;
    // True when DT_VERSYM/DT_VERDEF/DT_VERNEED metadata is declared. The
    // first symbol-lookup feature records the boundary but rejects versioned
    // name-only lookup until version matching has its own contract.
    bool has_symbol_versioning{};
};

struct Elf32LinkerMetadataResult {
    Elf32LinkerMetadataError error{Elf32LinkerMetadataError::None};
    Elf32LinkerMetadata metadata;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32LinkerMetadataError::None;
    }
};

// T001 semantic collection: classify the first supported dynamic-linker tag set
// while preserving pointer-like fields as raw dynamic values.
[[nodiscard]] Elf32CollectedLinkerMetadataResult collect_elf32_linker_metadata(
    std::span<const Elf32DynamicEntry> entries);

// Validated metadata: rebase pointer-like fields exactly once with the
// loader-provided load bias, validate declared guest ranges through GuestMemory,
// and enforce the supported ELF32 entry-size/string-offset invariants. DT_HASH
// and DT_GNU_HASH retain only validated fixed-header guest descriptors here;
// their variable arrays belong to elf32_symbol_lookup. This function is
// read-only and never changes mappings, permissions, or guest bytes.
[[nodiscard]] Elf32LinkerMetadataResult build_elf32_linker_metadata(
    const memory::GuestMemory& memory,
    std::uint32_t load_bias,
    std::span<const Elf32DynamicEntry> entries);

[[nodiscard]] const char* to_string(Elf32LinkerMetadataError error) noexcept;

}  // namespace liba32android::elf
