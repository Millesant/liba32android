#include "elf/elf32_linker_metadata.h"

#include <cstdint>
#include <optional>

namespace liba32android::elf {
namespace {

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

[[nodiscard]] Elf32LinkerMetadataResult failure(Elf32LinkerMetadataError error) {
    Elf32LinkerMetadataResult result;
    result.error = error;
    return result;
}

[[nodiscard]] bool assign_singleton(std::optional<std::uint32_t>& field,
                                    std::uint32_t value) {
    if (field.has_value()) return false;
    field = value;
    return true;
}

[[nodiscard]] bool pair_complete(const std::optional<std::uint32_t>& first,
                                 const std::optional<std::uint32_t>& second) {
    return first.has_value() == second.has_value();
}

}  // namespace

Elf32LinkerMetadataResult collect_elf32_linker_metadata(
    std::span<const Elf32DynamicEntry> entries) {
    std::optional<std::uint32_t> strtab;
    std::optional<std::uint32_t> strsz;
    std::optional<std::uint32_t> symtab;
    std::optional<std::uint32_t> syment;
    std::optional<std::uint32_t> rel;
    std::optional<std::uint32_t> relsz;
    std::optional<std::uint32_t> relent;
    std::optional<std::uint32_t> soname;

    Elf32LinkerMetadataResult result;

    for (const Elf32DynamicEntry& entry : entries) {
        if (entry.tag == kDtNull) break;

        bool accepted = true;
        switch (entry.tag) {
        case kDtNeeded:
            result.metadata.needed_offsets.push_back(entry.value);
            break;
        case kDtStrtab:
            accepted = assign_singleton(strtab, entry.value);
            break;
        case kDtSymtab:
            accepted = assign_singleton(symtab, entry.value);
            break;
        case kDtStrsz:
            accepted = assign_singleton(strsz, entry.value);
            break;
        case kDtSyment:
            accepted = assign_singleton(syment, entry.value);
            break;
        case kDtSoname:
            accepted = assign_singleton(soname, entry.value);
            break;
        case kDtRel:
            accepted = assign_singleton(rel, entry.value);
            break;
        case kDtRelsz:
            accepted = assign_singleton(relsz, entry.value);
            break;
        case kDtRelent:
            accepted = assign_singleton(relent, entry.value);
            break;
        default:
            // Structural parsing preserves deferred/unknown tags. Semantic
            // support is added only by explicit later linker slices.
            break;
        }

        if (!accepted) {
            return failure(Elf32LinkerMetadataError::DuplicateSingleton);
        }
    }

    if (!pair_complete(strtab, strsz) ||
        ((soname.has_value() || !result.metadata.needed_offsets.empty()) &&
         !strtab.has_value())) {
        return failure(Elf32LinkerMetadataError::IncompleteStringTable);
    }
    if (!pair_complete(symtab, syment)) {
        return failure(Elf32LinkerMetadataError::IncompleteSymbolTable);
    }

    const unsigned rel_fields =
        static_cast<unsigned>(rel.has_value()) +
        static_cast<unsigned>(relsz.has_value()) +
        static_cast<unsigned>(relent.has_value());
    if (rel_fields != 0 && rel_fields != 3) {
        return failure(Elf32LinkerMetadataError::IncompleteRelTable);
    }

    if (strtab.has_value()) {
        result.metadata.string_table = Elf32DynamicStringTableMetadata{
            .address_value = *strtab,
            .size = *strsz,
        };
    }
    if (symtab.has_value()) {
        result.metadata.symbol_table = Elf32DynamicSymbolTableMetadata{
            .address_value = *symtab,
            .entry_size = *syment,
        };
    }
    if (rel.has_value()) {
        result.metadata.rel_table = Elf32DynamicRelTableMetadata{
            .address_value = *rel,
            .size = *relsz,
            .entry_size = *relent,
        };
    }
    result.metadata.soname_offset = soname;
    return result;
}

const char* to_string(Elf32LinkerMetadataError error) noexcept {
    switch (error) {
    case Elf32LinkerMetadataError::None: return "none";
    case Elf32LinkerMetadataError::DuplicateSingleton: return "duplicate_singleton";
    case Elf32LinkerMetadataError::IncompleteStringTable: return "incomplete_string_table";
    case Elf32LinkerMetadataError::IncompleteSymbolTable: return "incomplete_symbol_table";
    case Elf32LinkerMetadataError::IncompleteRelTable: return "incomplete_rel_table";
    }
    return "unknown";
}

}  // namespace liba32android::elf
