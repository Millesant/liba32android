#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include "elf/elf32_linker_metadata.h"
#include "memory/guest_memory.h"

namespace {

using liba32android::elf::Elf32DynamicEntry;
using liba32android::elf::Elf32LinkerMetadataError;
using liba32android::elf::build_elf32_linker_metadata;
using liba32android::elf::collect_elf32_linker_metadata;
using liba32android::memory::LinearGuestMemory;

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
        {kDtStrtab, 0x100},
        {kDtStrsz, 0x80},
        {kDtSymtab, 0x200},
        {kDtSyment, 16},
        {kDtRel, 0x300},
        {kDtRelsz, 0x20},
        {kDtRelent, 8},
        {kDtSoname, 3},
        {kDtNeeded, 9},
        {kDtNeeded, 21},
        {kDtGnuHash, 0x400},
        {kDtNull, 0},
    };
}

int test_valid_collection() {
    auto entries = full_entries();
    const auto result = collect_elf32_linker_metadata(entries);
    if (!result) return fail("valid linker metadata collection failed");

    if (!result.metadata.string_table.has_value() ||
        result.metadata.string_table->address_value != 0x100 ||
        result.metadata.string_table->size != 0x80) {
        return fail("string-table metadata was not collected exactly");
    }
    if (!result.metadata.symbol_table.has_value() ||
        result.metadata.symbol_table->address_value != 0x200 ||
        result.metadata.symbol_table->entry_size != 16) {
        return fail("symbol-table metadata was not collected exactly");
    }
    if (!result.metadata.rel_table.has_value() ||
        result.metadata.rel_table->address_value != 0x300 ||
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
        Elf32DynamicEntry{kDtStrtab, 0x100},
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
        Elf32DynamicEntry{kDtSymtab, 0x200},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (collect_elf32_linker_metadata(symbol_only).error !=
        Elf32LinkerMetadataError::IncompleteSymbolTable) {
        return fail("incomplete SYMTAB/SYMENT pair was not rejected");
    }

    const std::array rel_partial{
        Elf32DynamicEntry{kDtRel, 0x300},
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
        Elf32DynamicEntry{kDtStrtab, 0x100},
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

int test_valid_rebasing_and_zero_bias() {
    auto entries = full_entries();

    LinearGuestMemory biased_memory(0x1000, 0x1000);
    const auto biased = build_elf32_linker_metadata(biased_memory, 0x1000, entries);
    if (!biased) return fail("valid rebased linker metadata failed");
    if (!biased.metadata.string_table.has_value() ||
        biased.metadata.string_table->guest_address != 0x1100 ||
        !biased.metadata.symbol_table.has_value() ||
        biased.metadata.symbol_table->guest_address != 0x1200 ||
        !biased.metadata.rel_table.has_value() ||
        biased.metadata.rel_table->guest_address != 0x1300) {
        return fail("pointer-like dynamic values were not rebased exactly once");
    }
    if (biased.metadata.needed_offsets != std::vector<std::uint32_t>{9, 21}) {
        return fail("validated metadata did not preserve DT_NEEDED order");
    }

    LinearGuestMemory fixed_memory(0x400, 0x100);
    const auto fixed = build_elf32_linker_metadata(fixed_memory, 0, entries);
    if (!fixed ||
        fixed.metadata.string_table->guest_address != 0x100 ||
        fixed.metadata.symbol_table->guest_address != 0x200 ||
        fixed.metadata.rel_table->guest_address != 0x300) {
        return fail("zero load bias did not preserve fixed guest addresses");
    }
    return 0;
}

int test_address_and_range_overflow() {
    LinearGuestMemory memory(0x100, 0);

    const std::array address_overflow{
        Elf32DynamicEntry{kDtStrtab, 0xfffffff0U},
        Elf32DynamicEntry{kDtStrsz, 1},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0x20, address_overflow).error !=
        Elf32LinkerMetadataError::AddressOverflow) {
        return fail("rebased pointer overflow was not rejected");
    }

    const std::array range_overflow{
        Elf32DynamicEntry{kDtStrtab, 0xfffffff0U},
        Elf32DynamicEntry{kDtStrsz, 0x20},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0, range_overflow).error !=
        Elf32LinkerMetadataError::RangeOverflow) {
        return fail("guest range overflow was not rejected");
    }
    return 0;
}

int test_unreadable_ranges() {
    LinearGuestMemory memory(0x100, 0x1000);

    const std::array bad_strings{
        Elf32DynamicEntry{kDtStrtab, 0x9000},
        Elf32DynamicEntry{kDtStrsz, 1},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0, bad_strings).error !=
        Elf32LinkerMetadataError::ReadFailed) {
        return fail("unreadable string table was not rejected");
    }

    const std::array bad_symbols{
        Elf32DynamicEntry{kDtSymtab, 0x9000},
        Elf32DynamicEntry{kDtSyment, 16},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0, bad_symbols).error !=
        Elf32LinkerMetadataError::ReadFailed) {
        return fail("unreadable symbol table was not rejected");
    }

    const std::array bad_rel{
        Elf32DynamicEntry{kDtRel, 0x9000},
        Elf32DynamicEntry{kDtRelsz, 8},
        Elf32DynamicEntry{kDtRelent, 8},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0, bad_rel).error !=
        Elf32LinkerMetadataError::ReadFailed) {
        return fail("unreadable REL table was not rejected");
    }
    return 0;
}

int test_entry_sizes_and_rel_size() {
    LinearGuestMemory memory(0x1000, 0x1000);

    const std::array bad_syment{
        Elf32DynamicEntry{kDtSymtab, 0x100},
        Elf32DynamicEntry{kDtSyment, 15},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0x1000, bad_syment).error !=
        Elf32LinkerMetadataError::InvalidSymbolEntrySize) {
        return fail("invalid DT_SYMENT was not rejected");
    }

    const std::array bad_relent{
        Elf32DynamicEntry{kDtRel, 0x300},
        Elf32DynamicEntry{kDtRelsz, 8},
        Elf32DynamicEntry{kDtRelent, 4},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0x1000, bad_relent).error !=
        Elf32LinkerMetadataError::InvalidRelEntrySize) {
        return fail("invalid DT_RELENT was not rejected");
    }

    const std::array bad_relsz{
        Elf32DynamicEntry{kDtRel, 0x300},
        Elf32DynamicEntry{kDtRelsz, 10},
        Elf32DynamicEntry{kDtRelent, 8},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0x1000, bad_relsz).error !=
        Elf32LinkerMetadataError::InvalidRelSize) {
        return fail("non-integral REL byte size was not rejected");
    }
    return 0;
}

int test_string_offsets() {
    LinearGuestMemory memory(0x1000, 0x1000);

    const std::array bad_soname{
        Elf32DynamicEntry{kDtStrtab, 0x100},
        Elf32DynamicEntry{kDtStrsz, 4},
        Elf32DynamicEntry{kDtSoname, 4},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0x1000, bad_soname).error !=
        Elf32LinkerMetadataError::StringOffsetOutOfRange) {
        return fail("out-of-range DT_SONAME offset was not rejected");
    }

    const std::array bad_needed{
        Elf32DynamicEntry{kDtStrtab, 0x100},
        Elf32DynamicEntry{kDtStrsz, 4},
        Elf32DynamicEntry{kDtNeeded, 4},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0x1000, bad_needed).error !=
        Elf32LinkerMetadataError::StringOffsetOutOfRange) {
        return fail("out-of-range DT_NEEDED offset was not rejected");
    }
    return 0;
}

int test_failure_does_not_mutate_guest_memory() {
    LinearGuestMemory memory(0x100, 0x1000);
    const std::array<std::uint8_t, 4> sentinel{0xde, 0xad, 0xbe, 0xef};
    if (!memory.write(0x1000, sentinel)) return fail("could not stage sentinel bytes");

    const std::array bad_syment{
        Elf32DynamicEntry{kDtSymtab, 0x10},
        Elf32DynamicEntry{kDtSyment, 15},
        Elf32DynamicEntry{kDtNull, 0},
    };
    if (build_elf32_linker_metadata(memory, 0x1000, bad_syment).error !=
        Elf32LinkerMetadataError::InvalidSymbolEntrySize) {
        return fail("mutation guard setup did not fail as expected");
    }

    std::array<std::uint8_t, 4> after{};
    if (!memory.read(0x1000, after) || after != sentinel) {
        return fail("failed metadata validation mutated guest memory");
    }
    return 0;
}

}  // namespace

int main() {
    if (const int status = test_valid_collection(); status != 0) return status;
    if (const int status = test_duplicate_singletons(); status != 0) return status;
    if (const int status = test_incomplete_groups(); status != 0) return status;
    if (const int status = test_deferred_tags_and_null_boundary(); status != 0) return status;
    if (const int status = test_valid_rebasing_and_zero_bias(); status != 0) return status;
    if (const int status = test_address_and_range_overflow(); status != 0) return status;
    if (const int status = test_unreadable_ranges(); status != 0) return status;
    if (const int status = test_entry_sizes_and_rel_size(); status != 0) return status;
    if (const int status = test_string_offsets(); status != 0) return status;
    if (const int status = test_failure_does_not_mutate_guest_memory(); status != 0) return status;
    return 0;
}
