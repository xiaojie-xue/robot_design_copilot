#include "robot_engine/protocol/frame.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

using robot_engine::protocol::FrameStatus;
using robot_engine::protocol::read_frame;
using robot_engine::protocol::write_frame;

void require(const bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

void test_round_trip() {
  constexpr auto payload = R"({"type":"request"})";
  std::stringstream stream;
  require(write_frame(stream, payload) == FrameStatus::ok,
          "write_frame accepts a valid payload");

  const auto result = read_frame(stream);
  require(result.ok(), "read_frame reads a valid payload");
  require(result.payload == payload, "round trip preserves payload bytes");
}

void test_multiple_frames() {
  std::stringstream stream;
  require(write_frame(stream, "one") == FrameStatus::ok, "write first frame");
  require(write_frame(stream, "two") == FrameStatus::ok, "write second frame");
  require(read_frame(stream).payload == "one", "read first frame");
  require(read_frame(stream).payload == "two", "read second frame");
  require(read_frame(stream).status == FrameStatus::end_of_stream,
          "clean EOF follows complete frames");
}

void test_truncated_header() {
  std::stringstream stream(std::string("\x00\x00", 2));
  require(read_frame(stream).status == FrameStatus::truncated_header,
          "partial header is rejected");
}

void test_empty_payload() {
  std::stringstream stream(std::string("\x00\x00\x00\x00", 4));
  require(read_frame(stream).status == FrameStatus::empty_payload,
          "zero-length input frame is rejected");

  std::stringstream output;
  require(write_frame(output, "") == FrameStatus::empty_payload,
          "zero-length output frame is rejected");
}

void test_oversized_payload() {
  std::stringstream stream(std::string("\x00\x00\x00\x05hello", 9));
  require(read_frame(stream, 4).status == FrameStatus::payload_too_large,
          "input above configured limit is rejected before allocation");

  std::stringstream output;
  require(write_frame(output, "hello", 4) == FrameStatus::payload_too_large,
          "output above configured limit is rejected");
}

void test_truncated_payload() {
  std::stringstream stream(std::string("\x00\x00\x00\x05hey", 7));
  require(read_frame(stream).status == FrameStatus::truncated_payload,
          "partial payload is rejected");
}

void test_write_failure() {
  std::ostringstream output;
  output.setstate(std::ios::badbit);
  require(write_frame(output, "payload") == FrameStatus::write_failed,
          "stream write failures are reported");
}

} // namespace

int main() {
  test_round_trip();
  test_multiple_frames();
  test_truncated_header();
  test_empty_payload();
  test_oversized_payload();
  test_truncated_payload();
  test_write_failure();
  std::cout << "All frame tests passed\n";
  return 0;
}
