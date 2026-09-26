#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "elf/elf32_dependency_graph.h"
#include "elf/elf32_linker_metadata.h"
#include "memory/guest_memory.h"

namespace liba32android::elf {

struct Elf32FunctionArrayDecodeOptions {
    std::uint32_t max_entries{};
};

enum class Elf32FunctionArrayDecodeError : std::uint8_t {
    None = 0,
    InvalidArraySize,
    RangeOverflow,
    TooManyEntries,
    ReadFailed,
};

struct Elf32FunctionArrayDecodeResult {
    Elf32FunctionArrayDecodeError error{Elf32FunctionArrayDecodeError::None};
    std::vector<std::uint32_t> entries;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32FunctionArrayDecodeError::None;
    }
};

// Decode one validated ELF32 INIT_ARRAY/FINI_ARRAY descriptor without guest
// mutation. Entries are returned as raw logical 32-bit values in declaration
// order. Sentinel filtering and function execution are lifecycle-policy work
// outside this primitive.
[[nodiscard]] Elf32FunctionArrayDecodeResult decode_elf32_function_array(
    const memory::GuestMemory& memory,
    const Elf32FunctionArrayMetadata& array,
    const Elf32FunctionArrayDecodeOptions& options);

[[nodiscard]] const char* to_string(
    Elf32FunctionArrayDecodeError error) noexcept;

struct Elf32InitCall {
    std::size_t object_index{};
    std::uint32_t array_index{};
    std::uint32_t function{};
};

struct Elf32InitPlanOptions {
    std::uint32_t max_objects{};
    // Total raw INIT_ARRAY entries decoded across all visited objects.
    // Sentinel entries consume this budget even though they are not calls.
    std::uint32_t max_entries{};
};

enum class Elf32InitPlanError : std::uint8_t {
    None = 0,
    InvalidOptions,
    InvalidRootObject,
    InvalidGraphEdge,
    ObjectLimitExceeded,
    EntryLimitExceeded,
    DecodeFailed,
};

struct Elf32InitPlanResult {
    Elf32InitPlanError error{Elf32InitPlanError::None};
    Elf32FunctionArrayDecodeError decode_error{
        Elf32FunctionArrayDecodeError::None};
    std::optional<std::size_t> failing_object;
    std::vector<Elf32InitCall> calls;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == Elf32InitPlanError::None;
    }
};

// Build one root-scoped constructor plan without guest execution. Reachable
// dependency objects contribute before their requester in stored edge order.
// Cycles/shared dependencies contribute each object at most once. Raw null and
// all-ones array entries are counted against max_entries but omitted as calls.
[[nodiscard]] Elf32InitPlanResult plan_elf32_init_array_calls(
    const memory::GuestMemory& memory,
    const Elf32DependencyGraph& graph,
    std::size_t root_object,
    const Elf32InitPlanOptions& options);

[[nodiscard]] const char* to_string(Elf32InitPlanError error) noexcept;

}  // namespace liba32android::elf
