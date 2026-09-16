#include "elf/elf32_loader.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace liba32android::elf {
namespace {

constexpr std::size_t kElf32HeaderSize = 52;
constexpr std::size_t kElf32ProgramHeaderSize = 32;
constexpr std::uint64_t kGuestAddressSpaceSize = std::uint64_t{1} << 32;

constexpr std::uint8_t kElfClass32 = 1;
constexpr std::uint8_t kElfDataLittleEndian = 1;
constexpr std::uint8_t kElfVersionCurrent = 1;
constexpr std::uint16_t kElfTypeExec = 2;
constexpr std::uint16_t kElfTypeDyn = 3;
constexpr std::uint16_t kElfMachineArm = 40;
constexpr std::uint32_t kProgramTypeLoad = 1;
constexpr std::uint32_t kFlagExecute = 1U << 0;
constexpr std::uint32_t kFlagWrite = 1U << 1;
constexpr std::uint32_t kFlagRead = 1U << 2;

struct RawLoadSegment {
    std::uint32_t offset{};
    std::uint32_t virtual_address{};
    std::uint32_t file_size{};
    std::uint32_t memory_size{};
    memory::MemoryPermission permissions{memory::MemoryPermission::None};
};

struct PlannedSegment {
    RawLoadSegment raw;
    std::uint32_t guest_address{};
    std::uint32_t mapping_start{};
    std::uint64_t mapping_end{};
};

[[nodiscard]] std::uint16_t read_u16(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(bytes[offset + 1]) << 8U;
}

[[nodiscard]] std::uint32_t read_u32(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset]) |
           static_cast<std::uint32_t>(bytes[offset + 1]) << 8U |
           static_cast<std::uint32_t>(bytes[offset + 2]) << 16U |
           static_cast<std::uint32_t>(bytes[offset + 3]) << 24U;
}

[[nodiscard]] std::uint64_t align_down(std::uint64_t value, std::uint64_t alignment) noexcept {
    return value - (value % alignment);
}

[[nodiscard]] bool align_up(std::uint64_t value, std::uint64_t alignment,
                            std::uint64_t& output) noexcept {
    const std::uint64_t remainder = value % alignment;
    if (remainder == 0) {
        output = value;
        return value <= kGuestAddressSpaceSize;
    }
    const std::uint64_t increment = alignment - remainder;
    if (value > kGuestAddressSpaceSize - increment) {
        return false;
    }
    output = value + increment;
    return true;
}

[[nodiscard]] bool is_power_of_two(std::uint32_t value) noexcept {
    return value != 0 && (value & (value - 1U)) == 0;
}

[[nodiscard]] bool decode_permissions(std::uint32_t flags,
                                      memory::MemoryPermission& permissions) noexcept {
    const bool read = (flags & kFlagRead) != 0;
    const bool write = (flags & kFlagWrite) != 0;
    const bool execute = (flags & kFlagExecute) != 0;

    // The first mapped backend supports the ordinary ELF segment shapes None,
    // R, RW and RX. W/X without R and RWX are deliberately rejected rather
    // than silently granting broader host-page permissions.
    if ((!read && (write || execute)) || (write && execute)) {
        return false;
    }

    permissions = memory::MemoryPermission::None;
    if (read) {
        permissions = permissions | memory::MemoryPermission::Read;
    }
    if (write) {
        permissions = permissions | memory::MemoryPermission::Write;
    }
    if (execute) {
        permissions = permissions | memory::MemoryPermission::Execute;
    }
    return true;
}

[[nodiscard]] Elf32LoadResult failure(Elf32LoadError error) {
    Elf32LoadResult result;
    result.error = error;
    return result;
}

}  // namespace

Elf32LoadResult load_elf32(memory::MappedGuestMemory& memory,
                           std::span<const std::uint8_t> image,
                           const Elf32LoadOptions& options) {
    if (image.size() < kElf32HeaderSize) {
        return failure(Elf32LoadError::TruncatedHeader);
    }

    if (image[0] != 0x7f || image[1] != 'E' || image[2] != 'L' || image[3] != 'F') {
        return failure(Elf32LoadError::BadMagic);
    }
    if (image[4] != kElfClass32) {
        return failure(Elf32LoadError::UnsupportedClass);
    }
    if (image[5] != kElfDataLittleEndian) {
        return failure(Elf32LoadError::UnsupportedEndian);
    }
    if (image[6] != kElfVersionCurrent) {
        return failure(Elf32LoadError::UnsupportedIdentVersion);
    }

    const std::uint16_t type = read_u16(image, 16);
    const std::uint16_t machine = read_u16(image, 18);
    const std::uint32_t version = read_u32(image, 20);
    const std::uint32_t entry = read_u32(image, 24);
    const std::uint32_t program_header_offset = read_u32(image, 28);
    const std::uint16_t header_size = read_u16(image, 40);
    const std::uint16_t program_header_entry_size = read_u16(image, 42);
    const std::uint16_t program_header_count = read_u16(image, 44);

    if (version != kElfVersionCurrent) {
        return failure(Elf32LoadError::UnsupportedElfVersion);
    }
    if (type != kElfTypeDyn && type != kElfTypeExec) {
        return failure(Elf32LoadError::UnsupportedType);
    }
    if (machine != kElfMachineArm) {
        return failure(Elf32LoadError::UnsupportedMachine);
    }
    if (header_size != kElf32HeaderSize) {
        return failure(Elf32LoadError::InvalidHeaderSize);
    }
    if (program_header_entry_size != kElf32ProgramHeaderSize) {
        return failure(Elf32LoadError::InvalidProgramHeaderSize);
    }

    const std::uint64_t program_header_bytes =
        static_cast<std::uint64_t>(program_header_entry_size) * program_header_count;
    const std::uint64_t program_header_end =
        static_cast<std::uint64_t>(program_header_offset) + program_header_bytes;
    if (program_header_end > image.size()) {
        return failure(Elf32LoadError::ProgramHeaderTableOutOfBounds);
    }

    const std::uint64_t page_size = memory.page_size();
    std::vector<RawLoadSegment> raw_segments;
    std::uint64_t minimum_page = kGuestAddressSpaceSize;
    std::uint64_t maximum_page_end = 0;

    for (std::uint16_t index = 0; index < program_header_count; ++index) {
        const std::size_t offset = static_cast<std::size_t>(program_header_offset) +
                                   static_cast<std::size_t>(index) * kElf32ProgramHeaderSize;
        if (read_u32(image, offset) != kProgramTypeLoad) {
            continue;
        }

        RawLoadSegment segment;
        segment.offset = read_u32(image, offset + 4);
        segment.virtual_address = read_u32(image, offset + 8);
        segment.file_size = read_u32(image, offset + 16);
        segment.memory_size = read_u32(image, offset + 20);
        const std::uint32_t flags = read_u32(image, offset + 24);
        const std::uint32_t alignment = read_u32(image, offset + 28);

        if (segment.file_size > segment.memory_size) {
            return failure(Elf32LoadError::SegmentFileszExceedsMemsz);
        }
        if (static_cast<std::uint64_t>(segment.offset) + segment.file_size > image.size()) {
            return failure(Elf32LoadError::SegmentFileOutOfBounds);
        }
        if (static_cast<std::uint64_t>(segment.virtual_address) + segment.memory_size >
            kGuestAddressSpaceSize) {
            return failure(Elf32LoadError::SegmentAddressOverflow);
        }
        if (alignment > 1 &&
            (!is_power_of_two(alignment) ||
             (segment.virtual_address % alignment) != (segment.offset % alignment))) {
            return failure(Elf32LoadError::SegmentAlignmentInvalid);
        }
        if (!decode_permissions(flags, segment.permissions)) {
            return failure(Elf32LoadError::UnsupportedSegmentPermissions);
        }

        raw_segments.push_back(segment);
        if (segment.memory_size == 0) {
            continue;
        }

        const std::uint64_t segment_start = segment.virtual_address;
        const std::uint64_t segment_end = segment_start + segment.memory_size;
        const std::uint64_t mapping_start = align_down(segment_start, page_size);
        std::uint64_t mapping_end = 0;
        if (!align_up(segment_end, page_size, mapping_end)) {
            return failure(Elf32LoadError::SegmentAddressOverflow);
        }
        minimum_page = std::min(minimum_page, mapping_start);
        maximum_page_end = std::max(maximum_page_end, mapping_end);
    }

    if (raw_segments.empty() || minimum_page == kGuestAddressSpaceSize) {
        return failure(Elf32LoadError::NoLoadSegments);
    }

    std::uint32_t load_bias = 0;
    if (type == kElfTypeDyn) {
        if (!options.dynamic_base.has_value()) {
            return failure(Elf32LoadError::DynamicBaseRequired);
        }
        const std::uint64_t dynamic_base = *options.dynamic_base;
        if ((dynamic_base % page_size) != 0) {
            return failure(Elf32LoadError::DynamicBaseUnaligned);
        }
        if (dynamic_base < minimum_page) {
            return failure(Elf32LoadError::LoadBiasOverflow);
        }
        const std::uint64_t bias = dynamic_base - minimum_page;
        if (bias > std::numeric_limits<std::uint32_t>::max() ||
            maximum_page_end + bias > kGuestAddressSpaceSize) {
            return failure(Elf32LoadError::LoadBiasOverflow);
        }
        load_bias = static_cast<std::uint32_t>(bias);
    }

    std::uint32_t loaded_entry = 0;
    if (entry != 0) {
        const std::uint64_t biased_entry = static_cast<std::uint64_t>(entry) + load_bias;
        if (biased_entry > std::numeric_limits<std::uint32_t>::max()) {
            return failure(Elf32LoadError::EntryAddressOverflow);
        }
        loaded_entry = static_cast<std::uint32_t>(biased_entry);
    }

    std::vector<PlannedSegment> planned_segments;
    planned_segments.reserve(raw_segments.size());
    for (const RawLoadSegment& raw : raw_segments) {
        if (raw.memory_size == 0) {
            continue;
        }
        const std::uint64_t guest_address = static_cast<std::uint64_t>(raw.virtual_address) + load_bias;
        const std::uint64_t guest_end = guest_address + raw.memory_size;
        if (guest_end > kGuestAddressSpaceSize) {
            return failure(Elf32LoadError::SegmentAddressOverflow);
        }

        PlannedSegment planned;
        planned.raw = raw;
        planned.guest_address = static_cast<std::uint32_t>(guest_address);
        planned.mapping_start = static_cast<std::uint32_t>(align_down(guest_address, page_size));
        if (!align_up(guest_end, page_size, planned.mapping_end)) {
            return failure(Elf32LoadError::SegmentAddressOverflow);
        }
        planned_segments.push_back(planned);
    }

    std::vector<std::size_t> order(planned_segments.size());
    for (std::size_t index = 0; index < order.size(); ++index) {
        order[index] = index;
    }
    std::sort(order.begin(), order.end(), [&](std::size_t lhs, std::size_t rhs) {
        return planned_segments[lhs].mapping_start < planned_segments[rhs].mapping_start;
    });
    for (std::size_t index = 1; index < order.size(); ++index) {
        const PlannedSegment& previous = planned_segments[order[index - 1]];
        const PlannedSegment& current = planned_segments[order[index]];
        if (static_cast<std::uint64_t>(current.mapping_start) < previous.mapping_end) {
            return failure(Elf32LoadError::SegmentPageOverlap);
        }
    }

    for (const PlannedSegment& planned : planned_segments) {
        for (std::uint64_t page = planned.mapping_start; page < planned.mapping_end;
             page += page_size) {
            if (memory.is_mapped(static_cast<std::uint32_t>(page))) {
                return failure(Elf32LoadError::AddressConflict);
            }
        }
    }

    struct MappedRange {
        std::uint32_t start{};
        std::size_t length{};
    };
    std::vector<MappedRange> mapped_ranges;
    mapped_ranges.reserve(planned_segments.size());

    const auto rollback = [&] {
        for (auto it = mapped_ranges.rbegin(); it != mapped_ranges.rend(); ++it) {
            (void)memory.unmap(it->start, it->length);
        }
    };

    const auto initialization_permissions =
        memory::MemoryPermission::Read | memory::MemoryPermission::Write;
    for (const PlannedSegment& planned : planned_segments) {
        const auto mapping_size = static_cast<std::size_t>(
            planned.mapping_end - static_cast<std::uint64_t>(planned.mapping_start));
        if (!memory.map(planned.mapping_start, mapping_size, initialization_permissions)) {
            rollback();
            return failure(Elf32LoadError::MapFailed);
        }
        mapped_ranges.push_back({planned.mapping_start, mapping_size});
    }

    constexpr std::size_t kZeroChunkSize = 4096;
    const std::array<std::uint8_t, kZeroChunkSize> zeroes{};
    for (const PlannedSegment& planned : planned_segments) {
        if (planned.raw.file_size != 0) {
            const auto file_bytes = image.subspan(planned.raw.offset, planned.raw.file_size);
            if (!memory.write(planned.guest_address, file_bytes)) {
                rollback();
                return failure(Elf32LoadError::WriteFailed);
            }
        }

        std::uint64_t zero_address = static_cast<std::uint64_t>(planned.guest_address) +
                                     planned.raw.file_size;
        std::uint64_t remaining = static_cast<std::uint64_t>(planned.raw.memory_size) -
                                  planned.raw.file_size;
        while (remaining != 0) {
            const std::size_t chunk = static_cast<std::size_t>(
                std::min<std::uint64_t>(remaining, zeroes.size()));
            if (!memory.write(static_cast<std::uint32_t>(zero_address),
                              std::span<const std::uint8_t>{zeroes.data(), chunk})) {
                rollback();
                return failure(Elf32LoadError::WriteFailed);
            }
            zero_address += chunk;
            remaining -= chunk;
        }
    }

    for (const PlannedSegment& planned : planned_segments) {
        const auto mapping_size = static_cast<std::size_t>(
            planned.mapping_end - static_cast<std::uint64_t>(planned.mapping_start));
        if (!memory.protect(planned.mapping_start, mapping_size, planned.raw.permissions)) {
            rollback();
            return failure(Elf32LoadError::ProtectFailed);
        }
    }

    Elf32LoadResult result;
    result.load_bias = load_bias;
    result.entry = loaded_entry;
    result.segments.reserve(planned_segments.size());
    for (const PlannedSegment& planned : planned_segments) {
        result.segments.push_back({
            .guest_address = planned.guest_address,
            .file_size = planned.raw.file_size,
            .memory_size = planned.raw.memory_size,
            .mapping_start = planned.mapping_start,
            .mapping_size = planned.mapping_end - static_cast<std::uint64_t>(planned.mapping_start),
            .permissions = planned.raw.permissions,
        });
    }
    return result;
}

const char* to_string(Elf32LoadError error) noexcept {
    switch (error) {
    case Elf32LoadError::None: return "none";
    case Elf32LoadError::TruncatedHeader: return "truncated_header";
    case Elf32LoadError::BadMagic: return "bad_magic";
    case Elf32LoadError::UnsupportedClass: return "unsupported_class";
    case Elf32LoadError::UnsupportedEndian: return "unsupported_endian";
    case Elf32LoadError::UnsupportedIdentVersion: return "unsupported_ident_version";
    case Elf32LoadError::UnsupportedElfVersion: return "unsupported_elf_version";
    case Elf32LoadError::UnsupportedType: return "unsupported_type";
    case Elf32LoadError::UnsupportedMachine: return "unsupported_machine";
    case Elf32LoadError::InvalidHeaderSize: return "invalid_header_size";
    case Elf32LoadError::InvalidProgramHeaderSize: return "invalid_program_header_size";
    case Elf32LoadError::ProgramHeaderTableOutOfBounds: return "program_header_table_out_of_bounds";
    case Elf32LoadError::NoLoadSegments: return "no_load_segments";
    case Elf32LoadError::SegmentFileszExceedsMemsz: return "segment_filesz_exceeds_memsz";
    case Elf32LoadError::SegmentFileOutOfBounds: return "segment_file_out_of_bounds";
    case Elf32LoadError::SegmentAddressOverflow: return "segment_address_overflow";
    case Elf32LoadError::SegmentAlignmentInvalid: return "segment_alignment_invalid";
    case Elf32LoadError::UnsupportedSegmentPermissions: return "unsupported_segment_permissions";
    case Elf32LoadError::DynamicBaseRequired: return "dynamic_base_required";
    case Elf32LoadError::DynamicBaseUnaligned: return "dynamic_base_unaligned";
    case Elf32LoadError::LoadBiasOverflow: return "load_bias_overflow";
    case Elf32LoadError::EntryAddressOverflow: return "entry_address_overflow";
    case Elf32LoadError::SegmentPageOverlap: return "segment_page_overlap";
    case Elf32LoadError::AddressConflict: return "address_conflict";
    case Elf32LoadError::MapFailed: return "map_failed";
    case Elf32LoadError::WriteFailed: return "write_failed";
    case Elf32LoadError::ProtectFailed: return "protect_failed";
    }
    return "unknown";
}

}  // namespace liba32android::elf
