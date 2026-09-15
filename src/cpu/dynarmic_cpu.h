#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace liba32android::cpu {

enum class InstructionSet {
    Arm,
    Thumb,
};

struct ExecutionResult {
    std::array<std::uint32_t, 16> regs{};
    std::uint32_t cpsr{};
    bool exception_raised{};
};

// M0-only execution seam. It deliberately exposes no ELF, Android, JNI or ABI
// behavior; those belong to later milestones and separate modules.
ExecutionResult execute_one(std::span<const std::uint8_t> code, InstructionSet instruction_set);

}  // namespace liba32android::cpu
