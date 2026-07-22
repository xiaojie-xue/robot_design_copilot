#include "robot_engine/protocol/handler.hpp"

#include "robot_engine/dependencies.hpp"
#include "robot_engine/design/calculation.hpp"

#ifdef ROBOT_ENGINE_WITH_ROBOTICS
#include "robot_engine/kinematics/reference_arm.hpp"
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#ifndef ROBOT_ENGINE_VERSION
#define ROBOT_ENGINE_VERSION "0.0.0-unknown"
#endif

namespace robot_engine::protocol {
namespace {

using Json = nlohmann::json;

constexpr std::size_t kMaxRequestIdLength = 128;
constexpr std::size_t kMaxMethodLength = 128;

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

[[nodiscard]] bool valid_method_text(const std::string_view text) {
  if (text.empty() || text.size() > kMaxMethodLength || text.front() < 'a' ||
      text.front() > 'z') {
    return false;
  }
  return std::all_of(text.begin(), text.end(), [](const char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9') || character == '.' ||
           character == '_' || character == '-';
  });
}

[[nodiscard]] bool valid_request_id(const Json &value) {
  return value.is_string() &&
         valid_request_id_text(value.get_ref<const std::string &>());
}

[[nodiscard]] Json request_id_or_null(const Json &message) {
  if (message.is_object()) {
    const auto iterator = message.find("request_id");
    if (iterator != message.end() && valid_request_id(*iterator)) {
      return *iterator;
    }
  }
  return nullptr;
}

[[nodiscard]] Json error_response(Json request_id, std::string code,
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

  return {
      {"type", "error"},
      {"request_id", std::move(request_id)},
      {"engine_version", engine_version()},
      {"error", std::move(error)},
  };
}

[[nodiscard]] Json validate_request(const Json &message) {
  if (!message.is_object()) {
    return error_response(nullptr, "invalid_request",
                          "The protocol message must be a JSON object.");
  }

  const auto request_id = request_id_or_null(message);
  if (!valid_request_id(message.value("request_id", Json{}))) {
    return error_response(
        nullptr, "invalid_request",
        "request_id must be an ASCII identifier of at most 128 bytes.");
  }
  if (!message.contains("type") || !message["type"].is_string() ||
      message["type"].get_ref<const std::string &>() != "request") {
    return error_response(request_id, "invalid_request",
                          "type must be request.");
  }
  if (!message.contains("method") || !message["method"].is_string() ||
      !valid_method_text(message["method"].get_ref<const std::string &>())) {
    return error_response(
        request_id, "invalid_request",
        "method must be a lowercase ASCII identifier of at most 128 bytes.");
  }
  if (!message.contains("params") || !message["params"].is_object()) {
    return error_response(request_id, "invalid_request",
                          "params must be a JSON object.");
  }

  constexpr std::array<std::string_view, 4> allowed_fields{"type", "request_id",
                                                           "method", "params"};
  for (auto iterator = message.begin(); iterator != message.end(); ++iterator) {
    if (std::find(allowed_fields.begin(), allowed_fields.end(),
                  iterator.key()) == allowed_fields.end()) {
      return error_response(request_id, "invalid_request",
                            "The request contains an unsupported field.", false,
                            {{"field", iterator.key()}});
    }
  }
  return Json{};
}

[[nodiscard]] Json dispatch_request(const Json &request) {
  const auto &request_id = request.at("request_id");
  const auto &method = request.at("method").get_ref<const std::string &>();

  if (method == "engine.health") {
    const auto versions = dependency_versions();
    const auto dependencies_ok = dependency_smoke_check();
    Json capabilities = Json::array({"engine.health", "design.calculate"});
    Json dependencies{
        {"eigen", versions.eigen},
        {"nlohmann_json", versions.nlohmann_json},
    };
#ifdef ROBOT_ENGINE_WITH_ROBOTICS
    capabilities.push_back("kinematics.forward");
    dependencies["pinocchio"] = versions.pinocchio;
    dependencies["ceres"] = versions.ceres;
#endif
    return {
        {"type", "response"},
        {"request_id", request_id},
        {"engine_version", engine_version()},
        {"result",
         {
             {"status", dependencies_ok ? "ok" : "degraded"},
             {"capabilities", std::move(capabilities)},
             {"dependencies", std::move(dependencies)},
             {"dependency_smoke_check", dependencies_ok},
         }},
    };
  }

  if (method == "design.calculate") {
    return {
        {"type", "response"},
        {"request_id", request_id},
        {"engine_version", engine_version()},
        {"result", design::calculate(request.at("params"))},
    };
  }

#ifdef ROBOT_ENGINE_WITH_ROBOTICS
  if (method == "kinematics.forward") {
    const auto &params = request.at("params");
    if (params.size() != 1 || !params.contains("joint_positions_rad") ||
        !params.at("joint_positions_rad").is_array()) {
      return error_response(
          request_id, "invalid_params",
          "params must contain only joint_positions_rad as an array.", false,
          {{"method", method}});
    }

    const auto &values = params.at("joint_positions_rad");
    if (values.size() != kinematics::kReferenceArmJointCount) {
      return error_response(
          request_id, "invalid_params",
          "joint_positions_rad must contain exactly seven values.", false,
          {{"expected_count", kinematics::kReferenceArmJointCount},
           {"received_count", values.size()}});
    }

    std::array<double, kinematics::kReferenceArmJointCount> joints{};
    for (std::size_t index = 0; index < values.size(); ++index) {
      if (!values[index].is_number()) {
        return error_response(
            request_id, "invalid_params",
            "Every joint position must be a finite JSON number.", false,
            {{"index", index}});
      }
      try {
        joints[index] = values[index].get<double>();
      } catch (const std::exception &) {
        return error_response(
            request_id, "invalid_params",
            "Every joint position must be representable as a finite double.",
            false, {{"index", index}});
      }
      if (!std::isfinite(joints[index])) {
        return error_response(
            request_id, "invalid_params",
            "Every joint position must be a finite JSON number.", false,
            {{"index", index}});
      }
    }

    const auto pose = kinematics::forward_reference_arm(joints);
    return {
        {"type", "response"},
        {"request_id", request_id},
        {"engine_version", engine_version()},
        {"result",
         {
             {"model_id", "reference_arm_7dof"},
             {"base_frame", "base"},
             {"tool_frame", "tool0"},
             {"joint_count", kinematics::kReferenceArmJointCount},
             {"translation_m", pose.translation_m},
             {"orientation_xyzw", pose.orientation_xyzw},
         }},
    };
  }
#endif

  return error_response(request_id, "method_not_found",
                        "The requested engine method is not available.", false,
                        {{"method", method}});
}

} // namespace

std::string handle_message(const std::string_view message) {
  try {
    const auto request = Json::parse(message.begin(), message.end());
    const auto validation_error = validate_request(request);
    if (!validation_error.is_null()) {
      return validation_error.dump();
    }
    return dispatch_request(request).dump();
  } catch (const nlohmann::json::parse_error &) {
    return error_response(nullptr, "invalid_json",
                          "The frame payload is not valid UTF-8 JSON.")
        .dump();
  } catch (const std::exception &) {
    return error_response(nullptr, "internal_error",
                          "The engine could not process the request.", true)
        .dump();
  }
}

std::string_view engine_version() noexcept { return ROBOT_ENGINE_VERSION; }

} // namespace robot_engine::protocol
