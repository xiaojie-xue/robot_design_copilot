#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>

namespace robot_engine::protocol {

inline constexpr std::size_t kFrameHeaderSize = 4;
inline constexpr std::size_t kDefaultMaxPayloadSize = 16U * 1024U * 1024U;

enum class FrameStatus {
  ok,
  end_of_stream,
  truncated_header,
  empty_payload,
  payload_too_large,
  truncated_payload,
  write_failed,
};

struct FrameResult {
  FrameStatus status{FrameStatus::end_of_stream};
  std::string payload;

  [[nodiscard]] bool ok() const noexcept { return status == FrameStatus::ok; }
};

// Reads a four-byte, unsigned, big-endian payload length followed by that many
// bytes. JSON validation belongs to the semantic protocol layer.
[[nodiscard]] FrameResult
read_frame(std::istream &input,
           std::size_t max_payload_size = kDefaultMaxPayloadSize);

// Writes one complete frame. Empty and oversized messages are rejected.
[[nodiscard]] FrameStatus
write_frame(std::ostream &output, std::string_view payload,
            std::size_t max_payload_size = kDefaultMaxPayloadSize);

[[nodiscard]] std::string_view frame_status_name(FrameStatus status) noexcept;

} // namespace robot_engine::protocol
