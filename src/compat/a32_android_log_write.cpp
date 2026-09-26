#include "compat/a32_android_log_write.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "memory/guest_memory.h"

namespace liba32android::compat {
namespace {

[[nodiscard]] bool read_guest_c_string(
    const memory::GuestMemory& memory,
    std::uint32_t address,
    std::size_t max_payload_bytes,
    std::string& output) {
    output.clear();

    for (std::size_t offset = 0;; ++offset) {
        if (offset >
            static_cast<std::size_t>(
                std::numeric_limits<std::uint32_t>::max() - address)) {
            return false;
        }

        std::array<std::uint8_t, 1> byte{};
        if (!memory.read(address + static_cast<std::uint32_t>(offset), byte)) {
            return false;
        }

        if (byte[0] == 0) {
            return true;
        }
        if (offset == max_payload_bytes) {
            return false;
        }

        output.push_back(static_cast<char>(byte[0]));
    }
}

}  // namespace

runtime::A32HostServiceDisposition A32AndroidLogWriteService::handle(
    memory::GuestMemory& memory,
    std::uint32_t svc_immediate,
    std::array<std::uint32_t, 16>& regs,
    std::uint32_t&) {
    if (svc_immediate != svc_immediate_) {
        return runtime::A32HostServiceDisposition::Unhandled;
    }

    const std::int32_t priority = std::bit_cast<std::int32_t>(regs[0]);
    const std::uint32_t tag_address = regs[1];
    const std::uint32_t text_address = regs[2];

    // Android accepts a null tag and substitutes platform/default policy.
    // A null text pointer is not a valid string argument for this bridge.
    std::optional<std::string> tag_storage;
    if (tag_address != 0) {
        tag_storage.emplace();
        if (!read_guest_c_string(
                memory, tag_address, options_.max_tag_bytes, *tag_storage)) {
            return runtime::A32HostServiceDisposition::Failed;
        }
    }

    if (text_address == 0) {
        return runtime::A32HostServiceDisposition::Failed;
    }
    std::string text;
    if (!read_guest_c_string(
            memory, text_address, options_.max_text_bytes, text)) {
        return runtime::A32HostServiceDisposition::Failed;
    }

    std::optional<std::string_view> tag;
    if (tag_storage.has_value()) {
        tag = std::string_view{*tag_storage};
    }

    const std::int32_t result =
        sink_.write(priority, tag, std::string_view{text});
    regs[0] = std::bit_cast<std::uint32_t>(result);
    return runtime::A32HostServiceDisposition::Handled;
}

}  // namespace liba32android::compat
