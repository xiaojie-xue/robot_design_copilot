#pragma once

#include <functional>
#include <memory>
#include <stop_token>
#include <string>
#include <string_view>

namespace robot_engine::protocol {

// Serializes complete protocol envelopes to the sidecar's framed output.
// Returning false records a terminal output failure and cancels in-flight work.
using MessageSink = std::function<bool(std::string_view)>;

// Long-running handlers report schema-valid progress through this callback and
// cooperate with cancellation by observing the supplied stop token. Executors
// may run concurrently and must not retain ProgressSink after returning.
using ProgressSink = std::function<void(double fraction, std::string_view stage,
                                        std::string_view message)>;
using RequestExecutor =
    std::function<std::string(std::string_view request, std::stop_token stop,
                              const ProgressSink &progress)>;

// Owns the concurrent request jobs for one stdin/stdout engine session.
// accept() is called by the single stdin reader. Request execution and output
// may happen concurrently; MessageSink calls are serialized by the session.
class EngineSession final {
public:
  explicit EngineSession(MessageSink sink,
                         RequestExecutor executor = RequestExecutor{});
  ~EngineSession();

  EngineSession(const EngineSession &) = delete;
  EngineSession &operator=(const EngineSession &) = delete;
  EngineSession(EngineSession &&) = delete;
  EngineSession &operator=(EngineSession &&) = delete;

  void accept(std::string_view message);
  // Stops accepting input and waits for accepted requests to complete.
  void finish();
  // Stops accepting input, requests cancellation, and waits for all workers.
  void shutdown();

  [[nodiscard]] bool output_ok() const noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace robot_engine::protocol
