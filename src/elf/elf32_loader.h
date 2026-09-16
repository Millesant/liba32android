#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "memory/guest_memory.h"

namespace liba32android::elf {

enum class Elf32LoadError : std::uint8_t {
    None = 0,
    TruncatedHeader,
    BadMagic,
    UnsupportedClass,
    UnsupportedEndian,
    UnsupportedIdentVersion,
    UnsupportedElfVersion,
    UnsupportedType,
    UnsupportedMachine,
    InvalidHeaderSize,
    InvalidProgramHeaderSize,
    ProgramHeaderTableOutOfBounds,
    NoLoadSegments,
    SegmentFileszExceedsMemsz,
    SegmentFileOutOfBounds,
    SegmentAddressOverflow,
    SegmentAlignmentInvalid,
    UnsupportedSegmentPermissions,
    DynamicBaseRequired,
    DynamicBaseUnaligned,
    LoadBiasOverflow,
    EntryAddressOverflow,
    SegmentPageOverlap,
    AddressConflict,
    MapFailed,
    WriteFailed,
    ProtectFailed,
};

struct Elf32LoadOptions {
    // For ET_DYN, this is the guest address where the lowest host-page-aligned
    // PT_LOAD mapping should begin. The resulting load bias must also preserve
    // every PT_LOAD p_align congruence requirement. ET_EXEC ignores this field
    // and loads at its fixed guest virtual addresses with load_bias == 0.
    std::optional<std::uint32_t> dynamic_base;
};

struct Elf32LoadedSegment {
    std::uint32_t guest_address{};
    std::uint32_t file_size{};
    std::uint32_t memory_size{};
    std::uint32_t mapping_start{};
    std::uint64_t mapping_size{};
    memory::MemoryPermission permissions{memory::MemoryPermission::None};
};

struct Elf32LoadResult {
    Elf32LoadError error{Elf32LoadError::None};
    std::uint32_t load_bias{};
    std::uint32_t entry{};
    std::vector<Elf32LoadedSegment> segments;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32LoadError::None;
    }
};

[[nodiscard]] Elf32LoadResult load_elf32(memory::MappedGuestMemory& memory,
                                         std::span<const std::uint8_t> image,
                                         const Elf32LoadOptions& options = {});

[[nodiscard]] const char* to_string(Elf32LoadError error) noexcept;

}  // namespace liba32android::elf
