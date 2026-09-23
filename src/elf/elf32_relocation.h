#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "elf/elf32_dependency_loader.h"
#include "elf/elf32_symbol_lookup.h"
#include "memory/guest_memory.h"

namespace liba32android::elf {

inline constexpr std::uint8_t kRArmNone = 0;
inline constexpr std::uint8_t kRArmAbs32 = 2;
inline constexpr std::uint8_t kRArmGlobDat = 21;
inline constexpr std::uint8_t kRArmRelative = 23;

// Caller-selected bounds for relocation work. T001 planning consumes only
// max_relocations. The symbol limits are carried here for the later
// resolution/application stages so one call surface owns all finite bounds.
struct Elf32RelocationOptions {
    std::uint32_t max_relocations{};
    Elf32SymbolLookupOptions symbols{};
};

struct Elf32RelocationEntry {
    std::uint32_t index{};
    std::uint32_t offset{};
    std::uint32_t info{};
    std::uint32_t symbol_index{};
    std::uint8_t type{};
    // Checked logical guest address: object load_bias + r_offset.
    // This is never a host pointer.
    std::uint32_t place_guest_address{};
    // Present only for supported write-producing relocations. This is the
    // original little-endian 32-bit place word: the REL addend where the
    // relocation formula uses A, and later the rollback snapshot.
    std::optional<std::uint32_t> original_word;
};

struct Elf32RelocationPlan {
    std::size_t object_index{};
    std::vector<Elf32RelocationEntry> entries;
};

enum class Elf32RelocationPlanError : std::uint8_t {
    None = 0,
    InvalidOptions,
    InvalidGraphObject,
    TooManyRelocations,
    RelocationReadFailed,
    PlaceOverflow,
    UnalignedPlace,
    TargetReadFailed,
    DuplicateTarget,
    UnsupportedRelocationType,
};

struct Elf32RelocationPlanResult {
    Elf32RelocationPlanError error{Elf32RelocationPlanError::None};
    Elf32RelocationPlan plan;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32RelocationPlanError::None;
    }
};

// Decode and validate one loaded object's main DT_REL table without mutating
// guest memory. The table descriptor is assumed to originate from validated
// Elf32LinkerMetadata (DT_RELENT == 8 and DT_RELSZ divisible by 8), but all
// guest bytes are still decoded explicitly through GuestMemory.
//
// Supported plan types are R_ARM_NONE, R_ARM_ABS32, R_ARM_GLOB_DAT and
// R_ARM_RELATIVE. Write-producing entries require a word-aligned, readable
// 32-bit place and capture its original word. R_ARM_NONE never reads its
// target. Duplicate write-producing places are rejected so future
// plan-before-mutate application never depends on an earlier relocation's
// write as another REL addend.
//
// Planning is bounded by max_relocations, is read-only, and returns only
// logical 32-bit guest addresses/values; no host pointer is exposed.
[[nodiscard]] Elf32RelocationPlanResult build_elf32_rel_relocation_plan(
    const memory::GuestMemory& memory,
    const Elf32DependencyGraph& graph,
    std::size_t object_index,
    const Elf32RelocationOptions& options);

[[nodiscard]] const char* to_string(Elf32RelocationPlanError error) noexcept;

}  // namespace liba32android::elf
