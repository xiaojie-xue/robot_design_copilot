#include "robot_engine/protocol/frame.hpp"

#include <array>
#include <istream>
#include <limits>
#include <ostream>
#include <utility>

namespace robot_engine::protocol {
namespace {

[[nodiscard]] std::uint32_t decode_length(
    const std::array<unsigned char, kFrameHeaderSize>& header) noexcept {
  return (static_cast<std::uint32_t>(header[0]) << 24U) |
         (static_cast<std::uint32_t>(header[1]) << 16U) |
         (static_cast<std::uint32_t>(header[2]) << 8U) |
         static_cast<std::uint32_t>(header[3]);
}

[[nodiscard]] std::array<char, kFrameHeaderSize> encode_length(
    const std::uint32_t length) noexcept {
  return {
      static_cast<char>((length >> 24U) & 0xFFU),
      static_cast<char>((length >> 16U) & 0xFFU),
      static_cast<char>((length >> 8U) & 0xFFU),
      static_cast<char>(length & 0xFFU),
  };
}

}  // namespace

FrameResult read_frame(std::istream& input, const std::size_t max_payload_size) {
  std::array<unsigned char, kFrameHeaderSize> header{};
  input.read(reinterpret_cast<char*>(header.data()),
             static_cast<std::streamsize>(header.size()));

  const auto header_bytes = input.gcount();
  if (header_bytes == 0 && input.eof()) {
    return {.status = FrameStatus::end_of_stream};
  }
  if (header_bytes != static_cast<std::streamsize>(header.size())) {
    return {.status = FrameStatus::truncated_header};
  }

  const auto payload_size = static_cast<std::size_t>(decode_length(header));
  if (payload_size == 0) {
    return {.status = FrameStatus::empty_payload};
  }
  if (payload_size > max_payload_size) {
    return {.status = FrameStatus::payload_too_large};
  }

  std::string payload(payload_size, '\0');
  input.read(payload.data(), static_cast<std::streamsize>(payload.size()));
  if (input.gcount() != static_cast<std::streamsize>(payload.size())) {
    return {.status = FrameStatus::truncated_payload};
  }

  return {.status = FrameStatus::ok, .payload = std::move(payload)};
}

FrameStatus write_frame(std::ostream& output,
                        const std::string_view payload,
                        const std::size_t max_payload_size) {
  if (payload.empty()) {
    return FrameStatus::empty_payload;
  }
  if (payload.size() > max_payload_size ||
      payload.size() > std::numeric_limits<std::uint32_t>::max()) {
    return FrameStatus::payload_too_large;
  }

  const auto header = encode_length(static_cast<std::uint32_t>(payload.size()));
  output.write(header.data(), static_cast<std::streamsize>(header.size()));
  output.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  output.flush();
  return output.good() ? FrameStatus::ok : FrameStatus::write_failed;
}

std::string_view frame_status_name(const FrameStatus status) noexcept {
  switch (status) {
    case FrameStatus::ok:
      return "ok";
    case FrameStatus::end_of_stream:
      return "end_of_stream";
    case FrameStatus::truncated_header:
      return "truncated_header";
    case FrameStatus::empty_payload:
      return "empty_payload";
    case FrameStatus::payload_too_large:
      return "payload_too_large";
    case FrameStatus::truncated_payload:
      return "truncated_payload";
    case FrameStatus::write_failed:
      return "write_failed";
  }
  return "unknown";
}

}  // namespace robot_engine::protocol
