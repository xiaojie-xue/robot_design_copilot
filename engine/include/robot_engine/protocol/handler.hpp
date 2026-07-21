#pragma once

#include <string>
#include <string_view>

namespace robot_engine::protocol {

inline constexpr int kProtocolVersion = 1;

// Parses one JSON protocol message and always returns a JSON response envelope.
// Framing and UTF-8 transport remain the caller's responsibility.
[[nodiscard]] std::string handle_message(std::string_view message);

[[nodiscard]] std::string_view engine_version() noexcept;

}  // namespace robot_engine::protocol
