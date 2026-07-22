#include "robot_engine/protocol/session.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <future>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

using Json = nlohmann::json;
using namespace std::chrono_literals;
using robot_engine::protocol::EngineSession;
using robot_engine::protocol::ProgressSink;

void require(const bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

class Collector {
public:
  bool write(const std::string_view envelope) {
    {
      const std::scoped_lock lock(mutex_);
      messages_.push_back(Json::parse(envelope));
    }
    changed_.notify_all();
    return true;
  }

  bool wait_for_size(const std::size_t size) {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, 2s,
                             [&] { return messages_.size() >= size; });
  }

  [[nodiscard]] std::vector<Json> snapshot() const {
    const std::scoped_lock lock(mutex_);
    return messages_;
  }

private:
  mutable std::mutex mutex_;
  std::condition_variable changed_;
  std::vector<Json> messages_;
};

std::string response_for(const std::string_view request) {
  const auto envelope = Json::parse(request);
  return Json{
      {"protocol_version", 1},
      {"type", "response"},
      {"request_id", envelope.at("request_id")},
      {"engine_version", "fixture"},
      {"result", {{"done", true}}},
  }
      .dump();
}

void test_progress_precedes_terminal_response() {
  Collector collector;
  EngineSession session(
      [&](const std::string_view message) { return collector.write(message); },
      [](const std::string_view request, std::stop_token,
         const ProgressSink &progress) {
        progress(0.5, "fixture", "halfway");
        return response_for(request);
      });

  session.accept(
      R"({"protocol_version":1,"type":"request","request_id":"progress-1","method":"engine.fixture","params":{}})");
  require(collector.wait_for_size(2),
          "progress request produces progress and a terminal response");
  session.shutdown();

  const auto messages = collector.snapshot();
  require(messages.size() == 2, "progress request produces exactly two events");
  require(messages[0]["type"] == "progress", "progress is emitted first");
  require(messages[0]["request_id"] == "progress-1",
          "progress preserves the request ID");
  require(messages[0]["progress"]["fraction"] == 0.5,
          "progress preserves the fraction");
  require(messages[0]["progress"]["stage"] == "fixture",
          "progress preserves the stage");
  require(messages[1]["type"] == "response", "request ends with one response");
  require(messages[1]["request_id"] == "progress-1",
          "response preserves the request ID");
}

void test_concurrent_responses_keep_request_ids() {
  Collector collector;
  std::promise<void> first_started;
  auto first_started_future = first_started.get_future();
  std::mutex release_mutex;
  std::condition_variable release_condition;
  bool release_first = false;

  EngineSession session(
      [&](const std::string_view message) { return collector.write(message); },
      [&](const std::string_view request, std::stop_token,
          const ProgressSink &) {
        const auto request_id =
            Json::parse(request).at("request_id").get<std::string>();
        if (request_id == "concurrent-1") {
          first_started.set_value();
          std::unique_lock lock(release_mutex);
          release_condition.wait(lock, [&] { return release_first; });
        }
        return response_for(request);
      });

  session.accept(
      R"({"protocol_version":1,"type":"request","request_id":"concurrent-1","method":"engine.fixture","params":{}})");
  require(first_started_future.wait_for(2s) == std::future_status::ready,
          "first concurrent request starts");
  session.accept(
      R"({"protocol_version":1,"type":"request","request_id":"concurrent-2","method":"engine.fixture","params":{}})");
  require(collector.wait_for_size(1),
          "second concurrent request can finish before the first");
  require(collector.snapshot()[0]["request_id"] == "concurrent-2",
          "out-of-order response keeps the second request ID");
  {
    const std::scoped_lock lock(release_mutex);
    release_first = true;
  }
  release_condition.notify_all();
  require(collector.wait_for_size(2), "first concurrent request can finish");
  session.shutdown();

  const auto messages = collector.snapshot();
  require(messages[1]["request_id"] == "concurrent-1",
          "out-of-order response keeps the first request ID");
}

void test_cancel_stops_in_flight_request() {
  Collector collector;
  std::promise<void> started;
  auto started_future = started.get_future();
  std::mutex wait_mutex;
  std::condition_variable_any wait_condition;

  EngineSession session(
      [&](const std::string_view message) { return collector.write(message); },
      [&](const std::string_view request, const std::stop_token stop,
          const ProgressSink &progress) {
        progress(0.1, "waiting", "ready for cancellation");
        started.set_value();
        std::unique_lock lock(wait_mutex);
        wait_condition.wait(lock, stop, [] { return false; });
        return response_for(request);
      });

  session.accept(
      R"({"protocol_version":1,"type":"request","request_id":"cancel-1","method":"engine.fixture","params":{}})");
  require(started_future.wait_for(2s) == std::future_status::ready,
          "cancellable request starts");
  session.accept(
      R"({"protocol_version":1,"type":"cancel","request_id":"cancel-1"})");
  require(collector.wait_for_size(2),
          "cancelled request produces a terminal error");
  session.shutdown();

  const auto messages = collector.snapshot();
  require(messages.size() == 2,
          "cancel envelope does not produce a separate acknowledgement");
  require(messages[0]["type"] == "progress",
          "request can report progress before cancellation");
  require(messages[1]["type"] == "error",
          "cancelled request completes with an error");
  require(messages[1]["request_id"] == "cancel-1",
          "cancellation error preserves the request ID");
  require(messages[1]["error"]["code"] == "request_cancelled",
          "cancellation uses a stable error code");
}

void test_duplicate_in_flight_request_is_rejected() {
  Collector collector;
  std::promise<void> started;
  auto started_future = started.get_future();
  std::mutex wait_mutex;
  std::condition_variable_any wait_condition;

  EngineSession session(
      [&](const std::string_view message) { return collector.write(message); },
      [&](const std::string_view request, const std::stop_token stop,
          const ProgressSink &) {
        started.set_value();
        std::unique_lock lock(wait_mutex);
        wait_condition.wait(lock, stop, [] { return false; });
        return response_for(request);
      });

  constexpr auto request =
      R"({"protocol_version":1,"type":"request","request_id":"duplicate-1","method":"engine.fixture","params":{}})";
  session.accept(request);
  require(started_future.wait_for(2s) == std::future_status::ready,
          "first duplicate request starts");
  session.accept(request);
  session.accept(
      R"({"protocol_version":1,"type":"cancel","request_id":"duplicate-1"})");
  require(collector.wait_for_size(2),
          "duplicate and cancelled original both complete");
  session.shutdown();

  const auto messages = collector.snapshot();
  require(messages.size() == 2,
          "duplicate request produces one error beside original completion");
  require(messages[0]["error"]["code"] == "duplicate_request",
          "duplicate request is rejected deterministically");
  require(messages[1]["error"]["code"] == "request_cancelled",
          "original duplicate request remains cancellable");
}

void test_invalid_cancel_is_rejected() {
  Collector collector;
  EngineSession session(
      [&](const std::string_view message) { return collector.write(message); });

  session.accept(
      R"({"protocol_version":1,"type":"cancel","request_id":"cancel-invalid","future":true})");
  require(collector.wait_for_size(1), "invalid cancel produces an error");
  session.shutdown();

  const auto messages = collector.snapshot();
  require(messages.size() == 1, "invalid cancel produces exactly one error");
  require(messages[0]["request_id"] == "cancel-invalid",
          "invalid cancel preserves a valid request ID");
  require(messages[0]["error"]["code"] == "invalid_request",
          "invalid cancel uses the request validation error code");
  require(messages[0]["error"]["details"]["field"] == "future",
          "invalid cancel identifies its unsupported field");
}

void test_output_failure_becomes_terminal() {
  EngineSession session([](std::string_view) { return false; });
  session.accept("{");
  require(!session.output_ok(), "output failure is retained by the session");
  session.shutdown();
}

} // namespace

int main() {
  test_progress_precedes_terminal_response();
  test_concurrent_responses_keep_request_ids();
  test_cancel_stops_in_flight_request();
  test_duplicate_in_flight_request_is_rejected();
  test_invalid_cancel_is_rejected();
  test_output_failure_becomes_terminal();
  std::cout << "All engine session tests passed\n";
  return 0;
}
