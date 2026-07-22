#include "robot_engine/protocol/frame.hpp"
#include "robot_engine/protocol/session.hpp"

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

} // namespace

int main() {
  using robot_engine::protocol::EngineSession;
  using robot_engine::protocol::frame_status_name;
  using robot_engine::protocol::FrameStatus;
  using robot_engine::protocol::read_frame;
  using robot_engine::protocol::write_frame;

  configure_binary_standard_streams();

  EngineSession session([](const std::string_view envelope) {
    return write_frame(std::cout, envelope) == FrameStatus::ok;
  });

  while (true) {
    auto frame = read_frame(std::cin);
    if (frame.status == FrameStatus::end_of_stream) {
      session.finish();
      return session.output_ok() ? 0 : 3;
    }
    if (!frame.ok()) {
      std::cerr << "protocol framing error: " << frame_status_name(frame.status)
                << '\n';
      return 2;
    }

    session.accept(frame.payload);
    if (!session.output_ok()) {
      std::cerr << "protocol write error\n";
      return 3;
    }
  }
}
