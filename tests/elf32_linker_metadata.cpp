#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "elf/elf32_linker_metadata.h"

namespace {

using liba32android::elf::Elf32DynamicEntry;
using liba32android::elf::Elf32LinkerMetadataError;
using liba32android::elf::collect_elf32_linker_metadata;

constexpr std::int32_t kDtNull = 0;
constexpr std::int32_t kDtNeeded = 1;
constexpr std::int32_t kDtStrtab = 5;
constexpr std::int32_t kDtSymtab = 6;
constexpr std::int32_t kDtStrsz = 10;
constexpr std::int32_t kDtSyment = 11;
constexpr std::int32_t kDtSoname = 14;
constexpr std::int32_t kDtRel = 17;
constexpr std::int32_t kDtRelsz = 18;
constexpr std::int32_t kDtRelent = 19;
constexpr std::int32_t kDtGnuHash = 0x6ffffef5;

int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
}

std::vector<Elf32DynamicEntry> full_entries() {
    return {
        {kDtStrtab, 0x1000},
        {kDtStrsz, 0x80},
        {kDtSymtab, 0x2000},
        {kDtSyment, 16},
        {kDtRel, 0x3000},
        {kDtRelsz, 0x20},
        {kDtRelent, 8},
        {kDtSoname, 3},
        {kDtNeeded, 9},
        {kDtNeeded, 21},
        {kDtGnuHash, 0x4000},
        {kDtNull, 0},
    };
}

int test_valid_collection() {
    auto entries = full_entries();
    const auto result = collect_elf32_linker_metadata(entries);
    if (!result) return fail("valid linker metadata collection failed");

    if (!result.metadata.string_table.has_value() ||
        result.metadata.string_table->address_value != 0x1000 ||
        result.metadata.string_table->size != 0x80) {
        return fail("string-table metadata was not collected exactly");
    }
    if (!result.metadata.symbol_table.has_value() ||
        result.metadata.symbol_table->address_value != 0x2000 ||
        result.metadata.symbol_table->entry_size != 16) {
        return fail("symbol-table metadata was not collected exactly");
    }
    if (!result.metadata.rel_table.has_value() ||
        result.metadata.rel_table->address_value != 0x3000 ||
        result.metadata.rel_table->size != 0x20 ||
        result.metadata.rel_table->entry_size != 8) {
        return fail("REL metadata was not collected exactly");
    }
    if (!result.metadata.soname_offset.has_value() ||
        *result.metadata.soname_offset != 3) {
        return fail("SONAME offset was not collected exactly");
    }
    if (result.metadata.needed_offsets != std::vector<std::uint32_t>{9, 21}) {
        return fail("DT_NEEDED offsets did not preserve dynamic-array order");
    }
    return 0;
}

int test_duplicate_singletons() {
    constexpr std::array<std::int32_t, 8> singleton_tags{
        kDtStrtab, kDtStrsz, kDtSymtab, kDtSyment,
        kDtRel, kDtRelsz, kDtRelent, kDtSoname,
    };

    for (const std::int32_t tag : singleton_tags) {
        auto entries = full_entries();
        entries.insert(entries.end() - 1, Elf32DynamicEntry{tag, 0xabcdef01U});
        if (collect_elf32_linker_metadata(entries).error !=
            Elf32LinkerMetadataError::DuplicateSingleton) {
            return fail("recognized singleton duplicate was not rejected");
        }
    }
    return 0;
}

int test_incomplete_groups() {
    const std::array string_only{
        Elf32DynamicEntry{kDtStrtab, 0x1000},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (collect_elf32_linker_metadata(string_only).error !=
        Elf32LinkerMetadataError::IncompleteStringTable) {
        return fail("incomplete STRTAB/STRSZ pair was not rejected");
    }

    const std::array needed_without_strings{
        Elf32DynamicEntry{kDtNeeded, 4},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (collect_elf32_linker_metadata(needed_without_strings).error !=
        Elf32LinkerMetadataError::IncompleteStringTable) {
        return fail("DT_NEEDED without a string table was not rejected");
    }

    const std::array symbol_only{
        Elf32DynamicEntry{kDtSymtab, 0x2000},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (collect_elf32_linker_metadata(symbol_only).error !=
        Elf32LinkerMetadataError::IncompleteSymbolTable) {
        return fail("incomplete SYMTAB/SYMENT pair was not rejected");
    }

    const std::array rel_partial{
        Elf32DynamicEntry{kDtRel, 0x3000},
        Elf32DynamicEntry{kDtRelsz, 0x20},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (collect_elf32_linker_metadata(rel_partial).error !=
        Elf32LinkerMetadataError::IncompleteRelTable) {
        return fail("incomplete REL/RELSZ/RELENT group was not rejected");
    }
    return 0;
}

int test_deferred_tags_and_null_boundary() {
    const std::array entries{
        Elf32DynamicEntry{kDtGnuHash, 0x12345678U},
        Elf32DynamicEntry{0x70000001, 0x87654321U},
        Elf32DynamicEntry{kDtNull, 0},
        // Entries after DT_NULL are outside the logical dynamic array and must
        // not affect collection even if a caller supplies them.
        Elf32DynamicEntry{kDtStrtab, 0x1000},
    };
    const auto result = collect_elf32_linker_metadata(entries);
    if (!result) return fail("deferred/unknown tags were rejected");
    if (result.metadata.string_table.has_value() ||
        result.metadata.symbol_table.has_value() ||
        result.metadata.rel_table.has_value() ||
        result.metadata.soname_offset.has_value() ||
        !result.metadata.needed_offsets.empty()) {
        return fail("semantic collection continued past DT_NULL");
    }
    return 0;
}

}  // namespace

int main() {
    if (const int status = test_valid_collection(); status != 0) return status;
    if (const int status = test_duplicate_singletons(); status != 0) return status;
    if (const int status = test_incomplete_groups(); status != 0) return status;
    if (const int status = test_deferred_tags_and_null_boundary(); status != 0) return status;
    return 0;
}
