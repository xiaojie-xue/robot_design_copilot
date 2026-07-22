#include "robot_engine/design/calculation.hpp"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#include <nlohmann/json.hpp>

#ifndef ROBOT_DESIGN_FIXTURE_DIR
#error "ROBOT_DESIGN_FIXTURE_DIR must point to design calculation fixtures"
#endif

namespace {

using Json = nlohmann::json;
using robot_engine::design::calculate;

constexpr double kAnalyticRelativeTolerance = 3e-3;
constexpr double kAnalyticAbsoluteToleranceNm = 2e-2;

void require(const bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

[[nodiscard]] Json load_fixture(const std::string &name) {
  std::ifstream stream(std::string(ROBOT_DESIGN_FIXTURE_DIR) + "/" + name);
  require(stream.good(), "open fixture " + name);
  return Json::parse(stream);
}

[[nodiscard]] bool near(const double actual, const double expected,
                        const double absolute = kAnalyticAbsoluteToleranceNm,
                        const double relative = kAnalyticRelativeTolerance) {
  return std::abs(actual - expected) <=
         absolute + relative * std::abs(expected);
}

[[nodiscard]] bool all_numbers_finite(const Json &value) {
  if (value.is_number_float()) {
    return std::isfinite(value.get<double>());
  }
  if (value.is_array() || value.is_object()) {
    for (const auto &child : value) {
      if (!all_numbers_finite(child)) {
        return false;
      }
    }
  }
  return true;
}

void test_feasible_chain_and_traceability() {
  const auto result = calculate(load_fixture("feasible-input.json"));
  require(result["status"] == "success", "feasible fixture succeeds");
  require(result["validation"]["status"] == "passed",
          "feasible fixture passes validation");
  require(result["workspace"]["coverage"] == 1.0,
          "all requested workspace targets are verified");
  require(result["workspace"]["targets"].size() == 2,
          "target-level FK/IK evidence is retained");
  require(result["dynamics"]["joint_requirements"].size() == 7,
          "all seven joints have sizing requirements");
  require(result["dynamics"]["evaluation_cases"].size() == 2,
          "declared limiting cases are retained");
  require(result["dynamics"]["mass_properties"]["links"].size() == 3,
          "solved geometry and link mass properties share one result");
  require(result["input_revision"] == "design-feasible",
          "input provenance is retained");
  require(result["solver_id"] == "design_solver",
          "solver provenance is retained");
  require(all_numbers_finite(result),
          "a successful result contains no non-finite number");

  for (const auto &target : result["workspace"]["targets"]) {
    require(target["position_residual_m"].get<double>() <=
                target["position_tolerance_m"].get<double>(),
            "FK position residual is inside the declared tolerance");
    require(target["orientation_residual_rad"].get<double>() <=
                target["orientation_tolerance_rad"].get<double>(),
            "FK orientation residual is inside the declared tolerance");
  }
}

void test_analytic_straight_arm_gravity_reference() {
  const auto result = calculate(load_fixture("feasible-input.json"));
  const auto reference = load_fixture("feasible-expected.json");
  require(result["status"] == "success", "analytic fixture succeeds");
  const auto &links = result["geometry"]["link_lengths"];
  const auto l1 = links[0]["value_m"].get<double>();
  const auto l2 = links[1]["value_m"].get<double>();
  const auto l3 = links[2]["value_m"].get<double>();
  const auto &expected_lengths = reference["expected"]["link_lengths_m"];
  const auto link_tolerance =
      reference["tolerances"]["link_length_abs_m"].get<double>();
  require(
      std::abs(l1 - expected_lengths[0].get<double>()) <= link_tolerance &&
          std::abs(l2 - expected_lengths[1].get<double>()) <= link_tolerance &&
          std::abs(l3 - expected_lengths[2].get<double>()) <= link_tolerance,
      "bounded optimization matches the analytic minimum reach geometry");
  const auto &expected =
      reference["expected"]["reach_x_gravity_torque_magnitudes_nm"];
  const auto &actual =
      result["dynamics"]["evaluation_cases"][0]["gravity_torque_nm"];
  require(near(std::abs(actual[1].get<double>()), expected[0].get<double>()),
          "joint 2 gravity matches the independent moment-arm reference");
  require(near(std::abs(actual[3].get<double>()), expected[1].get<double>()),
          "joint 4 gravity matches the independent moment-arm reference");
  require(near(std::abs(actual[5].get<double>()), expected[2].get<double>()),
          "joint 6 gravity matches the independent moment-arm reference");
}

void test_analytic_twist_mapping_and_power_consistency() {
  const auto result = calculate(load_fixture("feasible-input.json"));
  const auto reference = load_fixture("feasible-expected.json");
  const auto &lengths = result["geometry"]["link_lengths"];
  const auto l1 = lengths[0]["value_m"].get<double>();
  const auto l2 = lengths[1]["value_m"].get<double>();
  const auto l3 = lengths[2]["value_m"].get<double>();
  const auto &evaluation = result["dynamics"]["evaluation_cases"][0];
  const auto &velocity = evaluation["joint_velocities_rad_s"];
  const auto &acceleration = evaluation["joint_accelerations_rad_s2"];
  const auto tolerance =
      reference["tolerances"]["twist_reconstruction_abs"].get<double>();

  const auto reconstructed_linear_y =
      (l1 + l2 + l3) * velocity[0].get<double>() +
      (l2 + l3) * velocity[2].get<double>() + l3 * velocity[4].get<double>();
  const auto reconstructed_angular_z =
      velocity[0].get<double>() + velocity[2].get<double>() +
      velocity[4].get<double>() + velocity[6].get<double>();
  const auto reconstructed_linear_z =
      -((l1 + l2 + l3) * acceleration[1].get<double>() +
        (l2 + l3) * acceleration[3].get<double>() +
        l3 * acceleration[5].get<double>());
  const auto reconstructed_angular_y = acceleration[1].get<double>() +
                                       acceleration[3].get<double>() +
                                       acceleration[5].get<double>();
  require(std::abs(reconstructed_linear_y - 0.6) <= tolerance,
          "joint velocity reconstructs the declared TCP linear velocity");
  require(std::abs(reconstructed_angular_z - 1.2) <= tolerance,
          "joint velocity reconstructs the declared TCP angular velocity");
  require(
      std::abs(reconstructed_linear_z - 1.5) <= tolerance,
      "joint acceleration reconstructs the declared TCP linear acceleration");
  require(
      std::abs(reconstructed_angular_y - 2.5) <= tolerance,
      "joint acceleration reconstructs the declared TCP angular acceleration");

  for (const auto &case_result : result["dynamics"]["evaluation_cases"]) {
    for (std::size_t joint = 0; joint < 7; ++joint) {
      const auto expected_power =
          std::abs(case_result["required_torque_nm"][joint].get<double>() *
                   case_result["joint_velocities_rad_s"][joint].get<double>());
      require(near(case_result["mechanical_power_w"][joint].get<double>(),
                   expected_power, 1e-9, 1e-9),
              "mechanical power is consistent with torque times speed");
    }
  }
}

void test_infeasible_fixture_fails_closed() {
  const auto result = calculate(load_fixture("infeasible-input.json"));
  require(result["status"] == "infeasible",
          "outside-reach fixture is explicitly infeasible");
  require(result["validation"]["status"] == "failed",
          "infeasible result cannot carry passed validation");
  require(!result.contains("dynamics"),
          "infeasible geometry cannot produce sizing results");
}

void test_invalid_and_under_specified_inputs() {
  auto result = calculate(Json::array());
  require(result["status"] == "invalid_input",
          "a non-object contract is rejected without throwing");

  auto missing_mass = load_fixture("feasible-input.json");
  missing_mass.erase("mass_properties");
  result = calculate(missing_mass);
  require(result["status"] == "invalid_input",
          "missing mass properties are rejected");
  require(!result.contains("dynamics"),
          "missing mass properties cannot produce dynamics");

  auto missing_cases = load_fixture("feasible-input.json");
  missing_cases["evaluation_cases"] = Json::array();
  result = calculate(missing_cases);
  require(result["status"] == "invalid_input",
          "under-specified evaluation postures are rejected");

  auto non_finite = load_fixture("feasible-input.json");
  non_finite["mass_properties"]["payload"]["mass_kg"] =
      std::numeric_limits<double>::infinity();
  result = calculate(non_finite);
  require(result["status"] == "invalid_input",
          "non-finite engineering values are rejected");

  auto non_unit_orientation = load_fixture("feasible-input.json");
  non_unit_orientation["workspace"]["targets"][0]["orientation_xyzw"] =
      Json::array({0.0, 0.0, 0.0, 2.0});
  result = calculate(non_unit_orientation);
  require(result["status"] == "invalid_input",
          "non-unit orientations are rejected");
}

void test_iteration_limit_is_not_success() {
  auto input = load_fixture("feasible-input.json");
  input["solver"]["max_optimization_iterations"] = 1;
  input["solver"]["step_tolerance"] = 1e-12;
  const auto result = calculate(input);
  require(result["status"] == "non_converged",
          "iteration limit cannot be reported as success");
  require(!result.contains("dynamics"),
          "unconverged geometry cannot produce sizing results");
}

void test_reproducibility() {
  const auto input = load_fixture("feasible-input.json");
  require(calculate(input).dump() == calculate(input).dump(),
          "design calculation is deterministic for a fixed input revision");
}

} // namespace

int main() {
  test_feasible_chain_and_traceability();
  test_analytic_straight_arm_gravity_reference();
  test_analytic_twist_mapping_and_power_consistency();
  test_infeasible_fixture_fails_closed();
  test_invalid_and_under_specified_inputs();
  test_iteration_limit_is_not_success();
  test_reproducibility();
  std::cout << "All design calculation tests passed\n";
  return 0;
}
