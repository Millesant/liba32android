#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "cpu/a32_cpu.h"

namespace liba32android::memory {
class GuestMemory;
}

namespace liba32android::runtime {

enum class A32HostServiceDisposition : std::uint8_t {
    Handled = 0,
    Unhandled,
    Failed,
};

class A32HostServiceHandler {
public:
    virtual ~A32HostServiceHandler() = default;

    [[nodiscard]] virtual A32HostServiceDisposition handle(
        memory::GuestMemory& memory,
        std::uint32_t svc_immediate,
        std::array<std::uint32_t, 16>& regs,
        std::uint32_t& cpsr) = 0;
};

enum class A32ServiceDispatchError : std::uint8_t {
    None = 0,
    MemoryFault,
    CpuException,
    ServiceLimitExceeded,
    ServiceUnhandled,
    ServiceFailed,
    InstructionLimitExceeded,
};

struct A32ServiceDispatchResult {
    A32ServiceDispatchError error{A32ServiceDispatchError::None};
    std::array<std::uint32_t, 16> regs{};
    std::uint32_t cpsr{};
    std::size_t instructions_executed{};
    std::size_t services_handled{};
    bool stop_pc_reached{};
    std::optional<std::uint32_t> failing_svc_immediate;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == A32ServiceDispatchError::None;
    }
};

// Execute under one total guest-instruction budget while synchronously handling
// resumable SVC traps. request.instruction_count is the total budget across all
// CPU slices. Handler/register/memory side effects are never rolled back.
[[nodiscard]] A32ServiceDispatchResult execute_a32_with_services(
    memory::GuestMemory& memory,
    cpu::ExecutionRequest request,
    A32HostServiceHandler& handler,
    std::size_t max_service_calls);

[[nodiscard]] const char* to_string(
    A32ServiceDispatchError error) noexcept;

}  // namespace liba32android::runtime
