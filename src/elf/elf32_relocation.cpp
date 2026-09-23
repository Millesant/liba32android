#include "elf/elf32_relocation.h"

#include <array>
#include <cstdint>
#include <limits>
#include <unordered_set>

namespace liba32android::elf {
namespace {

constexpr std::uint32_t kElf32RelEntrySize = 8;
constexpr std::uint8_t kStbGlobal = 1;
constexpr std::uint8_t kStbWeak = 2;
constexpr std::uint8_t kSttNotype = 0;
constexpr std::uint8_t kSttObject = 1;
constexpr std::uint8_t kSttFunc = 2;
constexpr std::uint8_t kStvDefault = 0;
constexpr std::uint16_t kShnLoReserve = 0xff00;
constexpr std::uint16_t kShnAbs = 0xfff1;
constexpr std::uint16_t kShnCommon = 0xfff2;
constexpr std::uint16_t kShnXindex = 0xffff;

[[nodiscard]] Elf32RelocationPlanResult failure(
    Elf32RelocationPlanError error,
    std::size_t object_index) {
    Elf32RelocationPlanResult result;
    result.error = error;
    result.plan.object_index = object_index;
    return result;
}

[[nodiscard]] std::uint32_t decode_u32_le(
    const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

[[nodiscard]] bool checked_add(std::uint32_t base,
                               std::uint64_t offset,
                               std::uint32_t& result) noexcept {
    const std::uint64_t value = static_cast<std::uint64_t>(base) + offset;
    if (value > std::numeric_limits<std::uint32_t>::max()) return false;
    result = static_cast<std::uint32_t>(value);
    return true;
}

[[nodiscard]] bool read_word(const memory::GuestMemory& memory,
                             std::uint32_t address,
                             std::uint32_t& value) {
    std::array<std::uint8_t, 4> bytes{};
    if (!memory.read(address, bytes)) return false;
    value = decode_u32_le(bytes.data());
    return true;
}

[[nodiscard]] bool supported_type(std::uint8_t type) noexcept {
    return type == kRArmNone ||
           type == kRArmAbs32 ||
           type == kRArmGlobDat ||
           type == kRArmRelative;
}

[[nodiscard]] bool symbol_bearing_type(std::uint8_t type) noexcept {
    return type == kRArmAbs32 || type == kRArmGlobDat;
}

[[nodiscard]] bool valid_symbol_options(
    const Elf32SymbolLookupOptions& options) noexcept {
    return options.max_symbols != 0 &&
           options.max_hash_buckets != 0 &&
           options.max_gnu_bloom_words != 0 &&
           options.max_scope_objects != 0 &&
           options.max_name_bytes != 0;
}

[[nodiscard]] Elf32RelocationResolutionResult resolve_failure(
    Elf32RelocationResolveError error,
    std::size_t object_index,
    std::optional<std::uint32_t> failing_relocation = std::nullopt) {
    Elf32RelocationResolutionResult result;
    result.error = error;
    result.resolution.object_index = object_index;
    result.failing_relocation = failing_relocation;
    return result;
}

}  // namespace

Elf32RelocationPlanResult build_elf32_rel_relocation_plan(
    const memory::GuestMemory& memory,
    const Elf32DependencyGraph& graph,
    std::size_t object_index,
    const Elf32RelocationOptions& options) {
    if (object_index >= graph.objects.size()) {
        return failure(Elf32RelocationPlanError::InvalidGraphObject,
                       object_index);
    }

    Elf32RelocationPlanResult result;
    result.plan.object_index = object_index;

    const Elf32LoadedDependencyObject& object = graph.objects[object_index];
    if (!object.linker_metadata.rel_table.has_value()) {
        return result;
    }

    if (options.max_relocations == 0) {
        return failure(Elf32RelocationPlanError::InvalidOptions,
                       object_index);
    }

    const Elf32RelTableMetadata& table =
        *object.linker_metadata.rel_table;
    // Validated linker metadata guarantees these invariants. Keep a
    // defensive check because callers can construct the public metadata type
    // directly in tests/embedders.
    if (table.entry_size != kElf32RelEntrySize ||
        table.size % kElf32RelEntrySize != 0) {
        return failure(Elf32RelocationPlanError::RelocationReadFailed,
                       object_index);
    }

    const std::uint32_t count = table.size / kElf32RelEntrySize;
    if (count > options.max_relocations) {
        return failure(Elf32RelocationPlanError::TooManyRelocations,
                       object_index);
    }

    result.plan.entries.reserve(count);
    std::unordered_set<std::uint32_t> write_targets;
    write_targets.reserve(count);

    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t entry_address = 0;
        if (!checked_add(table.guest_address,
                         static_cast<std::uint64_t>(i) *
                             kElf32RelEntrySize,
                         entry_address)) {
            return failure(Elf32RelocationPlanError::RelocationReadFailed,
                           object_index);
        }

        std::array<std::uint8_t, kElf32RelEntrySize> bytes{};
        if (!memory.read(entry_address, bytes)) {
            return failure(Elf32RelocationPlanError::RelocationReadFailed,
                           object_index);
        }

        Elf32RelocationEntry entry;
        entry.index = i;
        entry.offset = decode_u32_le(bytes.data());
        entry.info = decode_u32_le(bytes.data() + 4);
        entry.symbol_index = entry.info >> 8U;
        entry.type = static_cast<std::uint8_t>(entry.info & 0xffU);

        if (!supported_type(entry.type)) {
            return failure(
                Elf32RelocationPlanError::UnsupportedRelocationType,
                object_index);
        }

        if (!checked_add(object.load.load_bias,
                         entry.offset,
                         entry.place_guest_address)) {
            return failure(Elf32RelocationPlanError::PlaceOverflow,
                           object_index);
        }

        if (entry.type != kRArmNone) {
            if ((entry.place_guest_address & 0x3U) != 0) {
                return failure(Elf32RelocationPlanError::UnalignedPlace,
                               object_index);
            }

            std::uint32_t original = 0;
            if (!read_word(memory, entry.place_guest_address, original)) {
                return failure(Elf32RelocationPlanError::TargetReadFailed,
                               object_index);
            }
            entry.original_word = original;

            if (!write_targets.insert(entry.place_guest_address).second) {
                return failure(Elf32RelocationPlanError::DuplicateTarget,
                               object_index);
            }
        }

        result.plan.entries.push_back(entry);
    }

    return result;
}

Elf32RelocationResolutionResult resolve_elf32_rel_relocation_references(
    const memory::GuestMemory& memory,
    const Elf32DependencyGraph& graph,
    std::size_t object_index,
    const Elf32RelocationOptions& options) {
    const Elf32RelocationPlanResult plan =
        build_elf32_rel_relocation_plan(memory, graph, object_index, options);
    if (!plan) {
        auto result = resolve_failure(
            Elf32RelocationResolveError::PlanFailed, object_index);
        result.plan_error = plan.error;
        return result;
    }

    Elf32RelocationResolutionResult result;
    result.resolution.object_index = object_index;
    result.resolution.entries.reserve(plan.plan.entries.size());

    bool needs_symbols = false;
    for (const Elf32RelocationEntry& entry : plan.plan.entries) {
        if (symbol_bearing_type(entry.type)) {
            needs_symbols = true;
            break;
        }
    }
    if (!needs_symbols) {
        for (const Elf32RelocationEntry& entry : plan.plan.entries) {
            result.resolution.entries.push_back(
                Elf32ResolvedRelocationEntry{.relocation = entry});
        }
        return result;
    }
    if (object_index >= graph.objects.size() ||
        !valid_symbol_options(options.symbols)) {
        return resolve_failure(
            Elf32RelocationResolveError::InvalidOptions, object_index);
    }

    const Elf32LoadedDependencyObject& object = graph.objects[object_index];
    if (object.linker_metadata.has_symbol_versioning) {
        return resolve_failure(
            Elf32RelocationResolveError::UnsupportedVersioning, object_index);
    }

    const Elf32SymbolIndexResult index =
        build_elf32_symbol_index(memory, object.linker_metadata,
                                 options.symbols);
    if (!index) {
        auto failed = resolve_failure(
            Elf32RelocationResolveError::IndexBuildFailed, object_index);
        failed.index_error = index.error;
        return failed;
    }

    for (const Elf32RelocationEntry& entry : plan.plan.entries) {
        Elf32ResolvedRelocationEntry resolved;
        resolved.relocation = entry;
        if (!symbol_bearing_type(entry.type)) {
            result.resolution.entries.push_back(std::move(resolved));
            continue;
        }
        if (entry.symbol_index == 0) {
            return resolve_failure(
                Elf32RelocationResolveError::MissingReferenceSymbol,
                object_index, entry.index);
        }
        if (entry.symbol_index >= index.index.symbol_count) {
            return resolve_failure(
                Elf32RelocationResolveError::SymbolIndexOutOfRange,
                object_index, entry.index);
        }

        const Elf32SymbolReadResult symbol_result =
            read_elf32_symbol_entry(memory, object.linker_metadata,
                                    index.index, entry.symbol_index);
        if (!symbol_result) {
            auto failed = resolve_failure(
                Elf32RelocationResolveError::ReferenceSymbolReadFailed,
                object_index, entry.index);
            failed.symbol_read_error = symbol_result.error;
            return failed;
        }
        const Elf32Symbol& symbol = symbol_result.symbol;

        if (!object.linker_metadata.string_table.has_value()) {
            auto failed = resolve_failure(
                Elf32RelocationResolveError::ReferenceNameFailed,
                object_index, entry.index);
            failed.string_error = Elf32LinkerStringError::MissingStringTable;
            return failed;
        }
        const Elf32SingleStringResult name =
            read_elf32_string_table_entry(
                memory, *object.linker_metadata.string_table,
                symbol.name_offset,
                Elf32LinkerStringOptions{
                    .max_string_bytes = options.symbols.max_name_bytes,
                });
        if (!name || name.value.empty()) {
            auto failed = resolve_failure(
                Elf32RelocationResolveError::ReferenceNameFailed,
                object_index, entry.index);
            failed.string_error = name.error;
            return failed;
        }

        if (symbol.binding != kStbGlobal && symbol.binding != kStbWeak) {
            return resolve_failure(
                Elf32RelocationResolveError::UnsupportedReferenceBinding,
                object_index, entry.index);
        }
        if ((symbol.raw_other & 0xfcU) != 0 ||
            symbol.visibility != kStvDefault) {
            return resolve_failure(
                Elf32RelocationResolveError::UnsupportedReferenceVisibility,
                object_index, entry.index);
        }
        if (symbol.type != kSttNotype &&
            symbol.type != kSttObject &&
            symbol.type != kSttFunc) {
            return resolve_failure(
                Elf32RelocationResolveError::UnsupportedReferenceType,
                object_index, entry.index);
        }
        if (symbol.section_index == kShnCommon ||
            symbol.section_index == kShnXindex ||
            (symbol.section_index >= kShnLoReserve &&
             symbol.section_index != kShnAbs)) {
            return resolve_failure(
                Elf32RelocationResolveError::UnsupportedReferenceSection,
                object_index, entry.index);
        }

        Elf32RelocationReference reference;
        reference.symbol_index = entry.symbol_index;
        reference.name = name.value;
        reference.symbol = symbol;

        const Elf32GraphSymbolLookupResult lookup =
            lookup_elf32_graph_symbol(memory, graph, object_index,
                                      name.value, options.symbols);
        if (lookup) {
            reference.symbol_value = lookup.symbol.symbol.guest_value;
            reference.defining_object_index = lookup.symbol.object_index;
            reference.defining_symbol_index =
                lookup.symbol.symbol.symbol_index;
        } else if (lookup.error ==
                   Elf32GraphSymbolLookupError::SymbolNotFound) {
            if (symbol.binding != kStbWeak) {
                return resolve_failure(
                    Elf32RelocationResolveError::UnresolvedStrongSymbol,
                    object_index, entry.index);
            }
            reference.symbol_value = 0;
            reference.unresolved_weak = true;
        } else {
            auto failed = resolve_failure(
                Elf32RelocationResolveError::SymbolLookupFailed,
                object_index, entry.index);
            failed.graph_error = lookup.error;
            failed.index_error = lookup.index_error;
            failed.lookup_error = lookup.lookup_error;
            failed.string_error = lookup.string_error;
            failed.failing_object = lookup.failing_object;
            return failed;
        }

        resolved.reference = std::move(reference);
        result.resolution.entries.push_back(std::move(resolved));
    }
    return result;
}

const char* to_string(Elf32RelocationPlanError error) noexcept {
    switch (error) {
    case Elf32RelocationPlanError::None: return "none";
    case Elf32RelocationPlanError::InvalidOptions: return "invalid_options";
    case Elf32RelocationPlanError::InvalidGraphObject: return "invalid_graph_object";
    case Elf32RelocationPlanError::TooManyRelocations: return "too_many_relocations";
    case Elf32RelocationPlanError::RelocationReadFailed: return "relocation_read_failed";
    case Elf32RelocationPlanError::PlaceOverflow: return "place_overflow";
    case Elf32RelocationPlanError::UnalignedPlace: return "unaligned_place";
    case Elf32RelocationPlanError::TargetReadFailed: return "target_read_failed";
    case Elf32RelocationPlanError::DuplicateTarget: return "duplicate_target";
    case Elf32RelocationPlanError::UnsupportedRelocationType:
        return "unsupported_relocation_type";
    }
    return "unknown";
}

const char* to_string(Elf32RelocationResolveError error) noexcept {
    switch (error) {
    case Elf32RelocationResolveError::None: return "none";
    case Elf32RelocationResolveError::InvalidOptions: return "invalid_options";
    case Elf32RelocationResolveError::PlanFailed: return "plan_failed";
    case Elf32RelocationResolveError::IndexBuildFailed: return "index_build_failed";
    case Elf32RelocationResolveError::MissingReferenceSymbol: return "missing_reference_symbol";
    case Elf32RelocationResolveError::SymbolIndexOutOfRange: return "symbol_index_out_of_range";
    case Elf32RelocationResolveError::ReferenceSymbolReadFailed: return "reference_symbol_read_failed";
    case Elf32RelocationResolveError::ReferenceNameFailed: return "reference_name_failed";
    case Elf32RelocationResolveError::UnsupportedVersioning: return "unsupported_versioning";
    case Elf32RelocationResolveError::UnsupportedReferenceBinding: return "unsupported_reference_binding";
    case Elf32RelocationResolveError::UnsupportedReferenceVisibility: return "unsupported_reference_visibility";
    case Elf32RelocationResolveError::UnsupportedReferenceType: return "unsupported_reference_type";
    case Elf32RelocationResolveError::UnsupportedReferenceSection: return "unsupported_reference_section";
    case Elf32RelocationResolveError::SymbolLookupFailed: return "symbol_lookup_failed";
    case Elf32RelocationResolveError::UnresolvedStrongSymbol: return "unresolved_strong_symbol";
    }
    return "unknown";
}

}  // namespace liba32android::elf
