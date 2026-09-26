#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "runtime/a32_service_dispatch.h"

namespace liba32android::compat {

struct A32AndroidLogWriteOptions {
    // Maximum payload bytes, excluding the required terminating NUL.
    std::size_t max_tag_bytes{};
    std::size_t max_text_bytes{};
};

class A32AndroidLogSink {
public:
    virtual ~A32AndroidLogSink() = default;

    // tag == std::nullopt represents a guest null tag pointer.
    [[nodiscard]] virtual std::int32_t write(
        std::int32_t priority,
        std::optional<std::string_view> tag,
        std::string_view text) = 0;
};

class A32AndroidLogWriteService final
    : public runtime::A32HostServiceHandler {
public:
    A32AndroidLogWriteService(
        std::uint32_t svc_immediate,
        A32AndroidLogSink& sink,
        A32AndroidLogWriteOptions options) noexcept
        : svc_immediate_(svc_immediate), sink_(sink), options_(options) {}

    [[nodiscard]] runtime::A32HostServiceDisposition handle(
        memory::GuestMemory& memory,
        std::uint32_t svc_immediate,
        std::array<std::uint32_t, 16>& regs,
        std::uint32_t& cpsr) override;

    [[nodiscard]] std::uint32_t svc_immediate() const noexcept {
        return svc_immediate_;
    }

private:
    std::uint32_t svc_immediate_{};
    A32AndroidLogSink& sink_;
    A32AndroidLogWriteOptions options_;
};

}  // namespace liba32android::compat
