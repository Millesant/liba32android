#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace liba32android::memory {
class GuestMemory;
}

namespace liba32android::cpu {

enum class InstructionSet {
    Arm,
    Thumb,
};

struct ExecutionRequest {
    InstructionSet instruction_set{InstructionSet::Arm};
    std::uint32_t entry_pc{};
    std::array<std::uint32_t, 16> regs{};
    std::size_t instruction_count{1};
};

struct ExecutionResult {
    std::array<std::uint32_t, 16> regs{};
    std::uint32_t cpsr{};
    std::size_t instructions_executed{};
    bool exception_raised{};
    bool memory_fault{};

    // Internal diagnostics used to prove which CPU/memory path was configured
    // and whether a fastmem fault fell back to callbacks. These fields are not
    // a public runtime ABI.
    bool fastmem_enabled{};
    std::size_t code_read_callbacks{};
    std::size_t data_read_callbacks{};
    std::size_t data_write_callbacks{};
};

// Generic A32 execution seam. Dynarmic is an implementation detail in the
// corresponding .cpp and must not leak into callers or the memory subsystem.
ExecutionResult execute(memory::GuestMemory& memory, const ExecutionRequest& request);

}  // namespace liba32android::cpu
