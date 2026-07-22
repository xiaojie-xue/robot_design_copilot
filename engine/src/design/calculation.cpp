#include "robot_engine/design/calculation.hpp"

#include "design/calculation_internal.hpp"

#include <cmath>
#include <exception>
#include <string>
#include <utility>

#ifndef ROBOT_ENGINE_VERSION
#define ROBOT_ENGINE_VERSION "0.0.0-unknown"
#endif

namespace robot_engine::design {
namespace {

using internal::Json;

[[nodiscard]] Json base_result(const Json &input) {
  const auto input_revision = input.is_object() &&
                                      input.contains("input_revision") &&
                                      input["input_revision"].is_string()
                                  ? input["input_revision"]
                                  : Json(nullptr);
  Json result{
      {"input_revision", input_revision},
      {"solver_id", kSolverId},
      {"engine_version", ROBOT_ENGINE_VERSION},
      {"status", "invalid_input"},
      {"warnings", Json::array()},
  };
  if (input.is_object() && input.contains("units") &&
      input["units"].is_object()) {
    result["units"] = input["units"];
  }
  if (input.is_object() && input.contains("frames") &&
      input["frames"].is_object()) {
    result["frames"] = input["frames"];
  }
  return result;
}

} // namespace

Json calculate(const Json &input_json) {
  using namespace internal;
  auto result = base_result(input_json);
  Json issues = Json::array();
  Input input;
  bool parsed = false;
  try {
    parsed = parse_input(input_json, input, issues);
  } catch (const std::exception &error) {
    add_issue(issues, "", "invalid_value_representation",
              std::string("The input value cannot be represented safely: ") +
                  error.what());
  }
  if (!parsed) {
    result["status"] = "invalid_input";
    result["validation"] = {
        {"status", "failed"},
        {"checks", Json::array({
                       {{"id", "design_input_contract"}, {"passed", false}},
                   })},
        {"issues", std::move(issues)},
    };
    return result;
  }

  result["units"] = input_json["units"];
  result["frames"] = input_json["frames"];
  result["assumptions"] = Json::array({
      "The parametric arm uses coincident Z/Y wrist pairs in the ordered "
      "Z-Y-Z-Y-Z-Y-Z axis sequence.",
      "Designed links are straight +X offsets; declared mass and inertia "
      "remain fixed while bounded lengths are optimized.",
      "Joint acceleration mapping neglects Jdot*qdot and is a conservative "
      "posture evaluation set, not a time-parameterized trajectory.",
      "Required torque sums separately safety-factored absolute gravity and "
      "inertial components.",
  });

  auto length_solution = solve_link_lengths(input);
  if (length_solution.status ==
      LinkLengthSolveStatus::maximum_geometry_unreachable) {
    const auto outside_reach = provably_outside_max_reach(input);
    result["status"] = outside_reach ? "infeasible" : "non_converged";
    Json target_results = Json::array();
    std::size_t reachable_count = 0;
    for (std::size_t index = 0; index < input.targets.size(); ++index) {
      const auto &target = input.targets[index];
      const auto &ik = length_solution.verification.results[index];
      if (ik.converged) {
        ++reachable_count;
      }
      target_results.push_back({
          {"target_id", target.id},
          {"reachable", ik.converged},
          {"position_residual_m", ik.position_residual},
          {"orientation_residual_rad", ik.orientation_residual},
          {"termination_reason", ik.termination},
      });
    }
    result["solver"] = {
        {"status", outside_reach ? "infeasible" : "non_converged"},
        {"termination_reason", outside_reach
                                   ? "target_outside_maximum_bounded_reach"
                                   : "ik_failed_at_maximum_link_bounds"},
        {"iterations", 0},
    };
    result["workspace"] = {
        {"coverage", static_cast<double>(reachable_count) /
                         static_cast<double>(input.targets.size())},
        {"requested_target_count", input.targets.size()},
        {"reachable_target_count", reachable_count},
        {"targets", std::move(target_results)},
    };
    result["validation"] = {
        {"status", "failed"},
        {"checks", Json::array({
                       {{"id", "design_input_contract"}, {"passed", true}},
                       {{"id", "workspace_coverage"}, {"passed", false}},
                       {{"id", "finite_outputs"}, {"passed", true}},
                   })},
        {"issues", Json::array()},
    };
    return result;
  }

  if (length_solution.status == LinkLengthSolveStatus::iteration_limit) {
    result["status"] = "non_converged";
    result["solver"] = {
        {"status", "non_converged"},
        {"termination_reason", "optimization_iteration_limit"},
        {"iterations", length_solution.iterations},
        {"final_bracket_width",
         length_solution.upper_bound - length_solution.lower_bound},
    };
    result["validation"] = {
        {"status", "failed"},
        {"checks", Json::array({
                       {{"id", "design_input_contract"}, {"passed", true}},
                       {{"id", "optimizer_convergence"}, {"passed", false}},
                   })},
        {"issues", Json::array()},
    };
    return result;
  }

  if (length_solution.status ==
      LinkLengthSolveStatus::final_verification_failed) {
    result["status"] = "non_converged";
    result["solver"] = {
        {"status", "non_converged"},
        {"termination_reason", "final_candidate_verification_failed"},
        {"iterations", length_solution.iterations},
    };
    result["validation"] = {
        {"status", "failed"},
        {"checks", Json::array({
                       {{"id", "design_input_contract"}, {"passed", true}},
                       {{"id", "workspace_coverage"}, {"passed", false}},
                   })},
        {"issues", Json::array()},
    };
    return result;
  }

  const auto &geometry = length_solution.geometry;
  const auto &final_verification = length_solution.verification;

  Json lengths = Json::array();
  for (std::size_t index = 0; index < kDesignedLinkCount; ++index) {
    const auto &bound = input.bounds[index];
    const auto value = geometry.links[index];
    lengths.push_back({
        {"id", bound.id},
        {"value_m", value},
        {"min_m", bound.minimum},
        {"max_m", bound.maximum},
        {"active_bound",
         std::abs(value - bound.minimum) <= input.solver.step_tolerance
             ? Json("min")
             : (std::abs(value - bound.maximum) <= input.solver.step_tolerance
                    ? Json("max")
                    : Json(nullptr))},
    });
  }
  Json target_results = Json::array();
  for (std::size_t index = 0; index < input.targets.size(); ++index) {
    const auto &target = input.targets[index];
    const auto &ik = final_verification.results[index];
    target_results.push_back({
        {"target_id", target.id},
        {"reachable", true},
        {"joint_positions_rad", json_vector(ik.joints)},
        {"position_residual_m", ik.position_residual},
        {"orientation_residual_rad", ik.orientation_residual},
        {"position_tolerance_m", target.position_tolerance},
        {"orientation_tolerance_rad", target.orientation_tolerance},
        {"ik_iterations", ik.iterations},
        {"termination_reason", ik.termination},
    });
  }

  result["status"] = "success";
  result["solver"] = {
      {"status", "converged"},
      {"termination_reason", "bounded_scale_bracket_tolerance"},
      {"iterations", length_solution.iterations},
      {"final_bracket_width",
       length_solution.upper_bound - length_solution.lower_bound},
      {"objective", "minimum common normalized position within link bounds"},
      {"objective_value", length_solution.scale},
  };
  result["geometry"] = {
      {"model_id", "parametric_7dof"},
      {"base_height_m", geometry.base_height},
      {"link_lengths", std::move(lengths)},
  };
  result["workspace"] = {
      {"coverage", 1.0},
      {"requested_target_count", input.targets.size()},
      {"reachable_target_count", input.targets.size()},
      {"targets", std::move(target_results)},
  };
  auto dynamics = calculate_dynamics(input, geometry, final_verification,
                                     result["warnings"]);
  append_link_mass_provenance(dynamics, input, geometry);
  result["dynamics"] = std::move(dynamics);
  result["validation"] = {
      {"status", "passed"},
      {"checks", Json::array({
                     {{"id", "design_input_contract"}, {"passed", true}},
                     {{"id", "optimizer_convergence"}, {"passed", true}},
                     {{"id", "fk_ik_workspace_coverage"}, {"passed", true}},
                     {{"id", "geometry_feeds_dynamics"}, {"passed", true}},
                     {{"id", "finite_outputs"}, {"passed", true}},
                 })},
      {"issues", Json::array()},
  };
  return result;
}

} // namespace robot_engine::design
