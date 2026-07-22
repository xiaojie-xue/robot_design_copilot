#include "robot_engine/protocol/session.hpp"

#include "robot_engine/protocol/handler.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <exception>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace robot_engine::protocol {
namespace {

using Json = nlohmann::json;

constexpr std::size_t kMaxRequestIdLength = 128;

[[nodiscard]] bool ascii_alphanumeric(const unsigned char character) noexcept {
  return (character >= static_cast<unsigned char>('a') &&
          character <= static_cast<unsigned char>('z')) ||
         (character >= static_cast<unsigned char>('A') &&
          character <= static_cast<unsigned char>('Z')) ||
         (character >= static_cast<unsigned char>('0') &&
          character <= static_cast<unsigned char>('9'));
}

[[nodiscard]] bool valid_request_id_text(const std::string_view text) {
  if (text.empty() || text.size() > kMaxRequestIdLength ||
      !ascii_alphanumeric(static_cast<unsigned char>(text.front()))) {
    return false;
  }
  return std::all_of(text.begin(), text.end(), [](const char character) {
    const auto byte = static_cast<unsigned char>(character);
    return ascii_alphanumeric(byte) ||
           byte == static_cast<unsigned char>('.') ||
           byte == static_cast<unsigned char>('_') ||
           byte == static_cast<unsigned char>(':') ||
           byte == static_cast<unsigned char>('-');
  });
}

[[nodiscard]] Json request_id_or_null(const Json &message) {
  if (!message.is_object()) {
    return nullptr;
  }
  const auto iterator = message.find("request_id");
  if (iterator == message.end() || !iterator->is_string()) {
    return nullptr;
  }
  const auto &request_id = iterator->get_ref<const std::string &>();
  return valid_request_id_text(request_id) ? Json(request_id) : Json(nullptr);
}

[[nodiscard]] std::string error_response(Json request_id, std::string code,
                                         std::string message,
                                         const bool retryable = false,
                                         Json details = Json::object()) {
  Json error{
      {"code", std::move(code)},
      {"message", std::move(message)},
      {"retryable", retryable},
  };
  if (!details.empty()) {
    error["details"] = std::move(details);
  }
  return Json{
      {"type", "error"},
      {"request_id", std::move(request_id)},
      {"engine_version", engine_version()},
      {"error", std::move(error)},
  }
      .dump();
}

struct CancelEnvelope {
  std::string request_id;
  std::string validation_error;
};

[[nodiscard]] CancelEnvelope validate_cancel(const Json &message) {
  const auto request_id = request_id_or_null(message);
  if (request_id.is_null()) {
    return {"", error_response(nullptr, "invalid_request",
                               "request_id must be an ASCII identifier of at "
                               "most 128 bytes.")};
  }

  constexpr std::array<std::string_view, 2> allowed_fields{"type",
                                                           "request_id"};
  for (auto iterator = message.begin(); iterator != message.end(); ++iterator) {
    if (std::find(allowed_fields.begin(), allowed_fields.end(),
                  iterator.key()) == allowed_fields.end()) {
      return {"", error_response(request_id, "invalid_request",
                                 "The cancellation contains an unsupported "
                                 "field.",
                                 false, {{"field", iterator.key()}})};
    }
  }
  return {request_id.get<std::string>(), ""};
}

[[nodiscard]] std::string progress_envelope(const std::string_view request_id,
                                            const double fraction,
                                            const std::string_view stage,
                                            const std::string_view message) {
  Json progress{{"fraction", fraction}, {"stage", stage}};
  if (!message.empty()) {
    progress["message"] = message;
  }
  return Json{
      {"type", "progress"},
      {"request_id", request_id},
      {"progress", std::move(progress)},
  }
      .dump();
}

} // namespace

struct EngineSession::Impl {
  struct Job {
    std::stop_source stop;
    std::thread worker;
    std::atomic_bool complete{false};
  };

  explicit Impl(MessageSink output_sink, RequestExecutor request_executor)
      : sink(std::move(output_sink)), executor(std::move(request_executor)) {
    if (!executor) {
      executor = [](const std::string_view request, std::stop_token,
                    const ProgressSink &) { return handle_message(request); };
    }
  }

  MessageSink sink;
  RequestExecutor executor;
  mutable std::mutex jobs_mutex;
  std::unordered_map<std::string, std::shared_ptr<Job>> jobs;
  std::mutex sink_mutex;
  std::atomic_bool accepting{true};
  std::atomic_bool output_healthy{true};

  void emit(const std::string_view envelope) {
    if (!output_healthy.load(std::memory_order_acquire)) {
      return;
    }
    bool written = false;
    try {
      const std::scoped_lock lock(sink_mutex);
      written = sink && sink(envelope);
    } catch (const std::exception &) {
      written = false;
    } catch (...) {
      written = false;
    }
    if (!written) {
      output_healthy.store(false, std::memory_order_release);
      request_stop_all();
    }
  }

  void request_stop_all() {
    const std::scoped_lock lock(jobs_mutex);
    for (const auto &entry : jobs) {
      entry.second->stop.request_stop();
    }
  }

  void reap_completed() {
    std::vector<std::shared_ptr<Job>> completed;
    {
      const std::scoped_lock lock(jobs_mutex);
      for (auto iterator = jobs.begin(); iterator != jobs.end();) {
        if (iterator->second->complete.load(std::memory_order_acquire)) {
          completed.push_back(iterator->second);
          iterator = jobs.erase(iterator);
        } else {
          ++iterator;
        }
      }
    }
    for (const auto &job : completed) {
      if (job->worker.joinable()) {
        job->worker.join();
      }
    }
  }

  void accept(const std::string_view message) {
    if (!accepting.load(std::memory_order_acquire) ||
        !output_healthy.load(std::memory_order_acquire)) {
      return;
    }
    reap_completed();

    Json envelope;
    try {
      envelope = Json::parse(message.begin(), message.end());
    } catch (const nlohmann::json::parse_error &) {
      emit(handle_message(message));
      return;
    }

    const auto type =
        envelope.is_object() ? envelope.find("type") : envelope.end();
    if (type != envelope.end() && type->is_string() &&
        type->get_ref<const std::string &>() == "cancel") {
      const auto cancel = validate_cancel(envelope);
      if (!cancel.validation_error.empty()) {
        emit(cancel.validation_error);
        return;
      }
      const std::scoped_lock lock(jobs_mutex);
      const auto iterator = jobs.find(cancel.request_id);
      if (iterator != jobs.end() &&
          !iterator->second->complete.load(std::memory_order_acquire)) {
        iterator->second->stop.request_stop();
      }
      return;
    }

    const auto request_id_value = request_id_or_null(envelope);
    if (request_id_value.is_null()) {
      emit(handle_message(message));
      return;
    }
    const auto request_id = request_id_value.get<std::string>();
    auto job = std::make_shared<Job>();
    bool duplicate = false;
    bool start_failed = false;
    {
      const std::scoped_lock lock(jobs_mutex);
      if (jobs.contains(request_id)) {
        duplicate = true;
      } else {
        jobs.emplace(request_id, job);
        try {
          const auto stop = job->stop.get_token();
          const auto request = std::string(message);
          job->worker = std::thread([this, job, stop, request_id, request] {
            std::string response;
            try {
              const ProgressSink progress =
                  [this, stop, request_id](
                      const double fraction, const std::string_view stage,
                      const std::string_view progress_message) {
                    if (stop.stop_requested() || !std::isfinite(fraction) ||
                        fraction < 0.0 || fraction > 1.0 || stage.empty()) {
                      return;
                    }
                    emit(progress_envelope(request_id, fraction, stage,
                                           progress_message));
                  };
              response = executor(request, stop, progress);
            } catch (const std::exception &) {
              response = error_response(
                  request_id, "internal_error",
                  "The engine could not process the request.", true);
            } catch (...) {
              response = error_response(
                  request_id, "internal_error",
                  "The engine could not process the request.", true);
            }

            if (stop.stop_requested()) {
              response = error_response(request_id, "request_cancelled",
                                        "The request was cancelled.");
            }
            emit(response);
            job->complete.store(true, std::memory_order_release);
          });
        } catch (const std::exception &) {
          jobs.erase(request_id);
          start_failed = true;
        }
      }
    }
    if (duplicate) {
      emit(error_response(request_id, "duplicate_request",
                          "The request ID is already in flight."));
    } else if (start_failed) {
      emit(error_response(request_id, "engine_busy",
                          "The engine could not start the request.", true));
    }
  }

  void stop_and_join(const bool cancel_active) {
    if (!accepting.exchange(false, std::memory_order_acq_rel)) {
      return;
    }
    std::vector<std::shared_ptr<Job>> active;
    {
      const std::scoped_lock lock(jobs_mutex);
      active.reserve(jobs.size());
      for (const auto &entry : jobs) {
        if (cancel_active) {
          entry.second->stop.request_stop();
        }
        active.push_back(entry.second);
      }
      jobs.clear();
    }
    for (const auto &job : active) {
      if (job->worker.joinable()) {
        job->worker.join();
      }
    }
  }
};

EngineSession::EngineSession(MessageSink sink, RequestExecutor executor)
    : impl_(std::make_unique<Impl>(std::move(sink), std::move(executor))) {}

EngineSession::~EngineSession() { shutdown(); }

void EngineSession::accept(const std::string_view message) {
  impl_->accept(message);
}

void EngineSession::finish() { impl_->stop_and_join(false); }

void EngineSession::shutdown() { impl_->stop_and_join(true); }

bool EngineSession::output_ok() const noexcept {
  return impl_->output_healthy.load(std::memory_order_acquire);
}

} // namespace robot_engine::protocol
