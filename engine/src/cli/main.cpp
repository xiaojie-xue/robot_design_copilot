#include "robot_engine/protocol/frame.hpp"
#include "robot_engine/protocol/handler.hpp"

#include <iostream>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {

void configure_binary_standard_streams() {
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
}

}  // namespace

int main() {
  using robot_engine::protocol::FrameStatus;
  using robot_engine::protocol::frame_status_name;
  using robot_engine::protocol::handle_message;
  using robot_engine::protocol::read_frame;
  using robot_engine::protocol::write_frame;

  configure_binary_standard_streams();

  while (true) {
    auto frame = read_frame(std::cin);
    if (frame.status == FrameStatus::end_of_stream) {
      return 0;
    }
    if (!frame.ok()) {
      std::cerr << "protocol framing error: " << frame_status_name(frame.status)
                << '\n';
      return 2;
    }

    const auto response = handle_message(frame.payload);
    const auto write_status = write_frame(std::cout, response);
    if (write_status != FrameStatus::ok) {
      std::cerr << "protocol write error: " << frame_status_name(write_status)
                << '\n';
      return 3;
    }
  }
}
