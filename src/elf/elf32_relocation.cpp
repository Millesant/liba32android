#include "elf/elf32_relocation.h"

#include <array>
#include <cstdint>
#include <limits>
#include <unordered_set>

namespace liba32android::elf {
namespace {

constexpr std::uint32_t kElf32RelEntrySize = 8;

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

}  // namespace liba32android::elf
