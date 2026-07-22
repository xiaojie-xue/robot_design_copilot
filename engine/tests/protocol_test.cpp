#include "robot_engine/protocol/handler.hpp"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#ifndef ROBOT_PROTOCOL_FIXTURE_DIR
#error "ROBOT_PROTOCOL_FIXTURE_DIR must point to protocol examples"
#endif
#ifndef ROBOT_DESIGN_FIXTURE_DIR
#error "ROBOT_DESIGN_FIXTURE_DIR must point to design calculation fixtures"
#endif

namespace {

using Json = nlohmann::json;
using robot_engine::protocol::handle_message;

void require(const bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

Json handle(const std::string &input) {
  return Json::parse(handle_message(input));
}

Json load_fixture(const std::string &name) {
  std::ifstream input(std::string(ROBOT_PROTOCOL_FIXTURE_DIR) + "/" + name);
  require(input.good(), "open protocol fixture " + name);
  return Json::parse(input);
}

Json load_design_fixture(const std::string &name) {
  std::ifstream input(std::string(ROBOT_DESIGN_FIXTURE_DIR) + "/" + name);
  require(input.good(), "open design fixture " + name);
  return Json::parse(input);
}

void test_committed_examples() {
  const auto health_request = load_fixture("health-request.json");
#ifdef ROBOT_ENGINE_WITH_ROBOTICS
  const auto expected_health = load_fixture("health-robotics-response.json");
#else
  const auto expected_health = load_fixture("health-response.json");
#endif
  require(handle(health_request.dump()) == expected_health,
          "committed health response matches engine output");

  const auto expected_error = load_fixture("error-response.json");
  require(handle("{") == expected_error,
          "committed invalid-JSON response matches engine output");

#ifdef ROBOT_ENGINE_WITH_ROBOTICS
  const auto forward_request = load_fixture("forward-request.json");
  const auto expected_forward = load_fixture("forward-response.json");
  const auto actual_forward = handle(forward_request.dump());
  require(actual_forward["type"] == expected_forward["type"] &&
              actual_forward["request_id"] == expected_forward["request_id"] &&
              actual_forward["engine_version"] ==
                  expected_forward["engine_version"] &&
              actual_forward["result"]["model_id"] ==
                  expected_forward["result"]["model_id"] &&
              actual_forward["result"]["base_frame"] ==
                  expected_forward["result"]["base_frame"] &&
              actual_forward["result"]["tool_frame"] ==
                  expected_forward["result"]["tool_frame"] &&
              actual_forward["result"]["joint_count"] ==
                  expected_forward["result"]["joint_count"],
          "committed forward response metadata matches engine output");
  for (std::size_t index = 0; index < 3; ++index) {
    require(
        std::abs(
            actual_forward["result"]["translation_m"][index].get<double>() -
            expected_forward["result"]["translation_m"][index].get<double>()) <
            1e-12,
        "committed forward translation matches engine output");
  }
  for (std::size_t index = 0; index < 4; ++index) {
    require(
        std::abs(
            actual_forward["result"]["orientation_xyzw"][index].get<double>() -
            expected_forward["result"]["orientation_xyzw"][index]
                .get<double>()) < 1e-12,
        "committed forward orientation matches engine output");
  }
#endif
}

void test_design_calculation() {
  const auto input = load_design_fixture("feasible-input.json");
  const Json request{{"type", "request"},
                     {"request_id", "design-feasible"},
                     {"method", "design.calculate"},
                     {"params", input}};
  const auto response = handle(request.dump());
  require(response["type"] == "response",
          "design calculation uses the protocol response envelope");
  require(response["result"]["status"] == "success",
          "design protocol entry point completes the calculation chain");
  require(response["result"]["dynamics"]["joint_requirements"].size() == 7,
          "design protocol result retains all joint requirements");
}

void test_health() {
  const auto response = handle(
      R"({"type":"request","request_id":"health-1","method":"engine.health","params":{}})");
  require(response["type"] == "response", "health returns a response");
  require(response["request_id"] == "health-1", "health preserves request ID");
  require(response["result"]["status"] == "ok", "health reports ok");
  require(response["result"]["dependencies"]["eigen"] == "3.4.1",
          "health reports the linked Eigen version");
  require(response["result"]["dependencies"]["nlohmann_json"] == "3.12.0",
          "health reports the linked JSON version");
  require(response["result"]["dependency_smoke_check"] == true,
          "health reports successful dependency smoke check");
  require(response["engine_version"].is_string(),
          "health reports engine version");
  require(response["result"]["capabilities"].size() >= 2 &&
              response["result"]["capabilities"][1] == "design.calculate",
          "health advertises the design calculation capability");
#ifdef ROBOT_ENGINE_WITH_ROBOTICS
  require(response["result"]["dependencies"]["pinocchio"] == "3.8.0",
          "health reports the linked Pinocchio version");
  require(response["result"]["dependencies"]["ceres"] == "2.2.0",
          "health reports the linked Ceres version");
  require(response["result"]["capabilities"] ==
              Json::array(
                  {"engine.health", "design.calculate", "kinematics.forward"}),
          "health reports robotics capability");
#endif
}

#ifdef ROBOT_ENGINE_WITH_ROBOTICS
void test_forward_kinematics() {
  const auto response = handle(
      R"({"type":"request","request_id":"fk-1","method":"kinematics.forward","params":{"joint_positions_rad":[0,0,0,0,0,0,0]}})");
  require(response["type"] == "response",
          "forward kinematics returns a response");
  require(response["request_id"] == "fk-1",
          "forward kinematics preserves request ID");
  require(response["result"]["model_id"] == "reference_arm_7dof",
          "forward kinematics identifies the reference model");
  require(response["result"]["translation_m"].size() == 3,
          "forward kinematics returns a translation vector");
  require(response["result"]["orientation_xyzw"].size() == 4,
          "forward kinematics returns an explicit xyzw quaternion");
}

void test_invalid_forward_params() {
  const auto wrong_count = handle(
      R"({"type":"request","request_id":"fk-count","method":"kinematics.forward","params":{"joint_positions_rad":[0,0]}})");
  require(wrong_count["error"]["code"] == "invalid_params",
          "forward kinematics rejects the wrong joint count");
  require(wrong_count["error"]["details"]["expected_count"] == 7,
          "joint-count error declares the expected count");

  const auto wrong_type = handle(
      R"({"type":"request","request_id":"fk-type","method":"kinematics.forward","params":{"joint_positions_rad":[0,0,0,"bad",0,0,0]}})");
  require(wrong_type["error"]["code"] == "invalid_params",
          "forward kinematics rejects nonnumeric joints");
  require(wrong_type["error"]["details"]["index"] == 3,
          "joint-type error identifies the bad index");
}
#endif

void test_invalid_json() {
  const auto response = handle("{");
  require(response["type"] == "error", "invalid JSON returns an error");
  require(response["request_id"].is_null(),
          "unparseable JSON has no correlation ID");
  require(response["error"]["code"] == "invalid_json",
          "invalid JSON has a stable error code");
}

void test_invalid_utf8() {
  std::string input = R"({"type":"request","request_id":"bad-)";
  input.push_back(static_cast<char>(0xFF));
  input += R"(","method":"engine.health","params":{}})";
  const auto response = handle(input);
  require(response["error"]["code"] == "invalid_json",
          "invalid UTF-8 is rejected as invalid JSON");
}

void test_invalid_envelope() {
  const auto response = handle(
      R"({"type":"request","request_id":"","method":"engine.health","params":{}})");
  require(response["error"]["code"] == "invalid_request",
          "empty request ID is rejected");
  require(response["request_id"].is_null(),
          "invalid request ID is not reflected");
}

void test_invalid_identifier() {
  const auto response = handle(
      R"({"type":"request","request_id":"has space","method":"engine.health","params":{}})");
  require(response["error"]["code"] == "invalid_request",
          "request IDs outside the documented ASCII grammar are rejected");
  require(response["request_id"].is_null(),
          "invalid request IDs are not reflected");
}

void test_invalid_method() {
  const auto response = handle(
      R"({"type":"request","request_id":"method-format","method":"Engine.Health","params":{}})");
  require(response["request_id"] == "method-format",
          "method validation preserves a valid request ID");
  require(response["error"]["code"] == "invalid_request",
          "methods outside the documented grammar are rejected");
}

void test_invalid_params() {
  const auto response = handle(
      R"({"type":"request","request_id":"params-1","method":"engine.health","params":[]})");
  require(response["error"]["code"] == "invalid_request",
          "non-object params are rejected");
}

void test_unknown_field() {
  const auto response = handle(
      R"({"type":"request","request_id":"field-1","method":"engine.health","params":{},"future":true})");
  require(response["error"]["code"] == "invalid_request",
          "unknown request fields are rejected");
  require(response["error"]["details"]["field"] == "future",
          "unknown-field errors identify the rejected field");
}

void test_unknown_method() {
  const auto response = handle(
      R"({"type":"request","request_id":"method-1","method":"robot.unknown","params":{}})");
  require(response["request_id"] == "method-1",
          "method error preserves request ID");
  require(response["error"]["code"] == "method_not_found",
          "unknown method has a stable error code");
  require(response["error"]["details"]["method"] == "robot.unknown",
          "method error identifies rejected method");
}

} // namespace

int main() {
  test_committed_examples();
  test_health();
  test_design_calculation();
#ifdef ROBOT_ENGINE_WITH_ROBOTICS
  test_forward_kinematics();
  test_invalid_forward_params();
#endif
  test_invalid_json();
  test_invalid_utf8();
  test_invalid_envelope();
  test_invalid_identifier();
  test_invalid_method();
  test_invalid_params();
  test_unknown_field();
  test_unknown_method();
  std::cout << "All protocol tests passed\n";
  return 0;
}
