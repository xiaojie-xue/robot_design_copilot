#include "design/calculation_internal.hpp"

#include <cmath>
#include <exception>
#include <set>
#include <string_view>
#include <utility>

namespace robot_engine::design::internal {

void add_issue(Json &issues, const std::string &path, const std::string &code,
               const std::string &message) {
  issues.push_back({{"path", path}, {"code", code}, {"message", message}});
}

[[nodiscard]] bool finite_number(const Json &value) {
  if (!value.is_number()) {
    return false;
  }
  try {
    return std::isfinite(value.get<double>());
  } catch (const std::exception &) {
    return false;
  }
}

bool read_number(const Json &parent, const char *field, const std::string &path,
                 double &output, Json &issues) {
  const auto iterator = parent.find(field);
  if (iterator == parent.end() || !finite_number(*iterator)) {
    add_issue(issues, path + "/" + field, "finite_number_required",
              "A finite JSON number is required.");
    return false;
  }
  output = iterator->get<double>();
  return true;
}

bool read_vector3(const Json &value, const std::string &path, Vector3 &output,
                  Json &issues, const bool unit_direction = false) {
  if (!value.is_array() || value.size() != 3) {
    add_issue(issues, path, "vector3_required",
              "Exactly three finite components are required.");
    return false;
  }
  for (Eigen::Index index = 0; index < 3; ++index) {
    if (!finite_number(value[static_cast<std::size_t>(index)])) {
      add_issue(issues, path + "/" + std::to_string(index),
                "finite_number_required", "A finite component is required.");
      return false;
    }
    output[index] = value[static_cast<std::size_t>(index)].get<double>();
  }
  if (unit_direction && std::abs(output.norm() - 1.0) > kDirectionTolerance) {
    add_issue(issues, path, "unit_vector_required",
              "Direction vectors must have unit length within 1e-6.");
    return false;
  }
  return true;
}

[[nodiscard]] bool exact_keys(const Json &value,
                              const std::set<std::string> &required,
                              const std::string &path, Json &issues) {
  if (!value.is_object()) {
    add_issue(issues, path, "object_required", "A JSON object is required.");
    return false;
  }
  bool valid = true;
  for (const auto &key : required) {
    if (!value.contains(key)) {
      add_issue(issues, path + "/" + key, "required",
                "The required field is missing.");
      valid = false;
    }
  }
  for (auto iterator = value.begin(); iterator != value.end(); ++iterator) {
    if (!required.contains(iterator.key())) {
      add_issue(issues, path + "/" + iterator.key(), "unsupported_field",
                "Unsupported fields are rejected to expose contract drift.");
      valid = false;
    }
  }
  return valid;
}

[[nodiscard]] bool parse_input(const Json &json, Input &input, Json &issues) {
  const std::set<std::string> top_level{"input_revision",
                                        "model_id",
                                        "units",
                                        "frames",
                                        "geometry",
                                        "workspace",
                                        "joint_limits",
                                        "mass_properties",
                                        "gravity_m_s2",
                                        "motion_limits",
                                        "evaluation_cases",
                                        "transmissions",
                                        "safety_factors",
                                        "solver"};
  if (!exact_keys(json, top_level, "", issues)) {
    return false;
  }
  if (!json["input_revision"].is_string() ||
      json["input_revision"].get_ref<const std::string &>().empty()) {
    add_issue(issues, "/input_revision", "nonempty_string_required",
              "input_revision must be a non-empty string.");
  } else {
    input.revision = json["input_revision"].get<std::string>();
  }
  if (!json["model_id"].is_string() || json["model_id"] != "parametric_7dof") {
    add_issue(issues, "/model_id", "unsupported_model",
              "The design calculation supports only parametric_7dof.");
  }

  const Json expected_units{{"length", "m"},   {"mass", "kg"},
                            {"angle", "rad"},  {"time", "s"},
                            {"torque", "N*m"}, {"power", "W"}};
  if (json["units"] != expected_units) {
    add_issue(
        issues, "/units", "si_units_required",
        "The normalized design contract requires the declared SI unit set.");
  }
  if (!exact_keys(json["frames"], {"base", "tool"}, "/frames", issues)) {
    return false;
  }
  if (!json["frames"]["base"].is_string() ||
      !json["frames"]["tool"].is_string() ||
      json["frames"]["base"].get_ref<const std::string &>().empty() ||
      json["frames"]["tool"].get_ref<const std::string &>().empty() ||
      json["frames"]["base"] == json["frames"]["tool"]) {
    add_issue(issues, "/frames", "distinct_frames_required",
              "Distinct non-empty base and tool frame IDs are required.");
  } else {
    input.base_frame = json["frames"]["base"].get<std::string>();
    input.tool_frame = json["frames"]["tool"].get<std::string>();
  }

  const auto &geometry = json["geometry"];
  if (!exact_keys(geometry, {"base_height_m", "link_length_bounds_m"},
                  "/geometry", issues)) {
    return false;
  }
  read_number(geometry, "base_height_m", "/geometry", input.base_height,
              issues);
  if (!(input.base_height >= 0.0)) {
    add_issue(issues, "/geometry/base_height_m", "nonnegative_required",
              "Base height cannot be negative.");
  }
  const auto &bounds = geometry["link_length_bounds_m"];
  if (!bounds.is_array() || bounds.size() != kDesignedLinkCount) {
    add_issue(issues, "/geometry/link_length_bounds_m", "exact_count_required",
              "Exactly upper_arm, forearm, and wrist bounds are required.");
  } else {
    constexpr std::array<std::string_view, 3> ids{"upper_arm", "forearm",
                                                  "wrist"};
    for (std::size_t index = 0; index < bounds.size(); ++index) {
      const auto path =
          "/geometry/link_length_bounds_m/" + std::to_string(index);
      if (!exact_keys(bounds[index], {"id", "min", "max"}, path, issues)) {
        continue;
      }
      auto &bound = input.bounds[index];
      if (!bounds[index]["id"].is_string() ||
          bounds[index]["id"] != std::string(ids[index])) {
        add_issue(issues, path + "/id", "ordered_link_id_required",
                  "Link IDs must be upper_arm, forearm, wrist in order.");
      } else {
        bound.id = std::string(ids[index]);
      }
      read_number(bounds[index], "min", path, bound.minimum, issues);
      read_number(bounds[index], "max", path, bound.maximum, issues);
      if (!(bound.minimum > 0.0 && bound.maximum >= bound.minimum)) {
        add_issue(issues, path, "invalid_bounds",
                  "Each link requires 0 < min <= max.");
      }
    }
  }

  const auto &joint_limits = json["joint_limits"];
  if (!joint_limits.is_array() || joint_limits.size() != kJointCount) {
    add_issue(issues, "/joint_limits", "exact_count_required",
              "Exactly seven ordered joint limits are required.");
  } else {
    for (std::size_t index = 0; index < kJointCount; ++index) {
      const auto path = "/joint_limits/" + std::to_string(index);
      if (!exact_keys(joint_limits[index],
                      {"id", "min_position_rad", "max_position_rad"}, path,
                      issues)) {
        continue;
      }
      auto &limit = input.joint_limits[index];
      const auto expected_id = "joint_" + std::to_string(index + 1);
      if (!joint_limits[index]["id"].is_string() ||
          joint_limits[index]["id"] != expected_id) {
        add_issue(issues, path + "/id", "ordered_joint_id_required",
                  "Joint IDs must be joint_1 through joint_7 in order.");
      }
      limit.id = expected_id;
      read_number(joint_limits[index], "min_position_rad", path, limit.minimum,
                  issues);
      read_number(joint_limits[index], "max_position_rad", path, limit.maximum,
                  issues);
      if (!(limit.minimum < limit.maximum)) {
        add_issue(issues, path, "invalid_bounds",
                  "Joint minimum must be less than maximum.");
      }
    }
  }

  const auto &workspace = json["workspace"];
  if (!exact_keys(workspace, {"targets"}, "/workspace", issues)) {
    return false;
  }
  const auto &targets = workspace["targets"];
  if (!targets.is_array() || targets.empty()) {
    add_issue(issues, "/workspace/targets", "nonempty_array_required",
              "At least one required target is needed.");
  } else {
    std::set<std::string> target_ids;
    for (std::size_t index = 0; index < targets.size(); ++index) {
      const auto path = "/workspace/targets/" + std::to_string(index);
      if (!exact_keys(targets[index],
                      {"id", "frame_id", "position_m", "orientation_xyzw",
                       "position_tolerance_m", "orientation_tolerance_rad"},
                      path, issues)) {
        continue;
      }
      Target target;
      if (!targets[index]["id"].is_string() ||
          targets[index]["id"].get_ref<const std::string &>().empty()) {
        add_issue(issues, path + "/id", "nonempty_string_required",
                  "Target IDs must be non-empty.");
      } else {
        target.id = targets[index]["id"].get<std::string>();
        if (!target_ids.insert(target.id).second) {
          add_issue(issues, path + "/id", "duplicate_id",
                    "Target IDs must be unique.");
        }
      }
      if (!targets[index]["frame_id"].is_string() ||
          targets[index]["frame_id"] != input.base_frame) {
        add_issue(issues, path + "/frame_id", "frame_mismatch",
                  "Workspace target positions must be expressed in base.");
      }
      read_vector3(targets[index]["position_m"], path + "/position_m",
                   target.position, issues);
      const auto &quaternion = targets[index]["orientation_xyzw"];
      if (!quaternion.is_array() || quaternion.size() != 4) {
        add_issue(issues, path + "/orientation_xyzw", "quaternion_required",
                  "A finite xyzw quaternion is required.");
      } else {
        Eigen::Vector4d xyzw;
        bool finite = true;
        for (Eigen::Index component = 0; component < 4; ++component) {
          if (!finite_number(quaternion[static_cast<std::size_t>(component)])) {
            finite = false;
            break;
          }
          xyzw[component] =
              quaternion[static_cast<std::size_t>(component)].get<double>();
        }
        if (!finite || std::abs(xyzw.norm() - 1.0) > kQuaternionTolerance) {
          add_issue(issues, path + "/orientation_xyzw",
                    "unit_quaternion_required",
                    "Quaternion norm must be one within 1e-6.");
        } else {
          target.orientation =
              Eigen::Quaterniond(xyzw[3], xyzw[0], xyzw[1], xyzw[2]);
        }
      }
      read_number(targets[index], "position_tolerance_m", path,
                  target.position_tolerance, issues);
      read_number(targets[index], "orientation_tolerance_rad", path,
                  target.orientation_tolerance, issues);
      if (!(target.position_tolerance > 0.0 &&
            target.orientation_tolerance > 0.0)) {
        add_issue(issues, path, "positive_tolerance_required",
                  "Target tolerances must be positive.");
      }
      input.targets.push_back(std::move(target));
    }
  }

  const auto &mass = json["mass_properties"];
  if (!exact_keys(mass, {"links", "payload"}, "/mass_properties", issues)) {
    return false;
  }
  const auto &links = mass["links"];
  if (!links.is_array() || links.size() != kDesignedLinkCount) {
    add_issue(issues, "/mass_properties/links", "exact_count_required",
              "Every designed link needs mass properties.");
  } else {
    for (std::size_t index = 0; index < links.size(); ++index) {
      const auto path = "/mass_properties/links/" + std::to_string(index);
      if (!exact_keys(
              links[index],
              {"id", "mass_kg", "com_fraction", "rotational_inertia_kg_m2"},
              path, issues)) {
        continue;
      }
      auto &link = input.link_masses[index];
      if (!links[index]["id"].is_string() ||
          links[index]["id"] != input.bounds[index].id) {
        add_issue(issues, path + "/id", "link_id_mismatch",
                  "Mass-property links must match geometry links in order.");
      }
      link.id = input.bounds[index].id;
      read_number(links[index], "mass_kg", path, link.mass, issues);
      read_number(links[index], "com_fraction", path, link.com_fraction,
                  issues);
      read_number(links[index], "rotational_inertia_kg_m2", path,
                  link.rotational_inertia, issues);
      if (!(link.mass > 0.0 && link.com_fraction > 0.0 &&
            link.com_fraction < 1.0 && link.rotational_inertia >= 0.0)) {
        add_issue(issues, path, "invalid_mass_properties",
                  "Link mass must be positive, COM inside (0,1), and inertia "
                  "nonnegative.");
      }
    }
  }
  const auto &payload = mass["payload"];
  if (!exact_keys(payload, {"mass_kg", "com_tool_m", "inertia_diagonal_kg_m2"},
                  "/mass_properties/payload", issues)) {
    return false;
  }
  read_number(payload, "mass_kg", "/mass_properties/payload",
              input.payload.mass, issues);
  read_vector3(payload["com_tool_m"], "/mass_properties/payload/com_tool_m",
               input.payload.com_tool, issues);
  read_vector3(payload["inertia_diagonal_kg_m2"],
               "/mass_properties/payload/inertia_diagonal_kg_m2",
               input.payload.inertia_diagonal, issues);
  if (!(input.payload.mass > 0.0) ||
      (input.payload.inertia_diagonal.array() < 0.0).any()) {
    add_issue(issues, "/mass_properties/payload", "invalid_mass_properties",
              "Payload mass must be positive and inertia nonnegative.");
  }

  read_vector3(json["gravity_m_s2"], "/gravity_m_s2", input.gravity, issues);
  if (!(input.gravity.norm() > 0.0)) {
    add_issue(issues, "/gravity_m_s2", "nonzero_gravity_required",
              "Gravity must be an explicit non-zero vector.");
  }

  const auto &motion = json["motion_limits"];
  if (!exact_keys(motion,
                  {"max_tcp_linear_velocity_m_s",
                   "max_tcp_angular_velocity_rad_s",
                   "max_tcp_linear_acceleration_m_s2",
                   "max_tcp_angular_acceleration_rad_s2"},
                  "/motion_limits", issues)) {
    return false;
  }
  read_number(motion, "max_tcp_linear_velocity_m_s", "/motion_limits",
              input.motion.linear_velocity, issues);
  read_number(motion, "max_tcp_angular_velocity_rad_s", "/motion_limits",
              input.motion.angular_velocity, issues);
  read_number(motion, "max_tcp_linear_acceleration_m_s2", "/motion_limits",
              input.motion.linear_acceleration, issues);
  read_number(motion, "max_tcp_angular_acceleration_rad_s2", "/motion_limits",
              input.motion.angular_acceleration, issues);
  if (!(input.motion.linear_velocity > 0.0 &&
        input.motion.angular_velocity > 0.0 &&
        input.motion.linear_acceleration > 0.0 &&
        input.motion.angular_acceleration > 0.0)) {
    add_issue(issues, "/motion_limits", "positive_limits_required",
              "Every TCP motion limit must be positive.");
  }

  const auto &cases = json["evaluation_cases"];
  if (!cases.is_array() || cases.empty()) {
    add_issue(issues, "/evaluation_cases", "nonempty_array_required",
              "At least one declared dynamics evaluation case is required.");
  } else {
    std::set<std::string> case_ids;
    std::set<std::string> target_ids;
    for (const auto &target : input.targets) {
      target_ids.insert(target.id);
    }
    for (std::size_t index = 0; index < cases.size(); ++index) {
      const auto path = "/evaluation_cases/" + std::to_string(index);
      if (!exact_keys(cases[index],
                      {"id", "target_id", "linear_velocity_direction_base",
                       "angular_velocity_axis_base",
                       "linear_acceleration_direction_base",
                       "angular_acceleration_axis_base"},
                      path, issues)) {
        continue;
      }
      EvaluationCase evaluation;
      if (!cases[index]["id"].is_string() ||
          cases[index]["id"].get_ref<const std::string &>().empty()) {
        add_issue(issues, path + "/id", "nonempty_string_required",
                  "Evaluation case IDs must be non-empty.");
      } else {
        evaluation.id = cases[index]["id"].get<std::string>();
        if (!case_ids.insert(evaluation.id).second) {
          add_issue(issues, path + "/id", "duplicate_id",
                    "Evaluation case IDs must be unique.");
        }
      }
      if (!cases[index]["target_id"].is_string() ||
          !target_ids.contains(cases[index]["target_id"].get<std::string>())) {
        add_issue(issues, path + "/target_id", "unknown_target",
                  "Each evaluation case must reference a workspace target.");
      } else {
        evaluation.target_id = cases[index]["target_id"].get<std::string>();
      }
      read_vector3(cases[index]["linear_velocity_direction_base"],
                   path + "/linear_velocity_direction_base",
                   evaluation.linear_velocity_direction, issues, true);
      read_vector3(cases[index]["angular_velocity_axis_base"],
                   path + "/angular_velocity_axis_base",
                   evaluation.angular_velocity_direction, issues, true);
      read_vector3(cases[index]["linear_acceleration_direction_base"],
                   path + "/linear_acceleration_direction_base",
                   evaluation.linear_acceleration_direction, issues, true);
      read_vector3(cases[index]["angular_acceleration_axis_base"],
                   path + "/angular_acceleration_axis_base",
                   evaluation.angular_acceleration_direction, issues, true);
      input.evaluation_cases.push_back(std::move(evaluation));
    }
  }

  const auto &transmissions = json["transmissions"];
  if (!transmissions.is_array() || transmissions.size() != kJointCount) {
    add_issue(issues, "/transmissions", "exact_count_required",
              "Exactly seven declared transmissions are required.");
  } else {
    for (std::size_t index = 0; index < transmissions.size(); ++index) {
      const auto path = "/transmissions/" + std::to_string(index);
      if (!exact_keys(transmissions[index], {"joint_id", "ratio", "efficiency"},
                      path, issues)) {
        continue;
      }
      auto &transmission = input.transmissions[index];
      const auto expected_id = "joint_" + std::to_string(index + 1);
      if (!transmissions[index]["joint_id"].is_string() ||
          transmissions[index]["joint_id"] != expected_id) {
        add_issue(issues, path + "/joint_id", "ordered_joint_id_required",
                  "Transmission joint IDs must be joint_1 through joint_7.");
      }
      transmission.joint_id = expected_id;
      read_number(transmissions[index], "ratio", path, transmission.ratio,
                  issues);
      read_number(transmissions[index], "efficiency", path,
                  transmission.efficiency, issues);
      if (!(transmission.ratio > 0.0 && transmission.efficiency > 0.0 &&
            transmission.efficiency <= 1.0)) {
        add_issue(issues, path, "invalid_transmission",
                  "Ratio must be positive and efficiency in (0,1].");
      }
    }
  }

  const auto &safety = json["safety_factors"];
  if (!exact_keys(safety, {"gravity", "dynamic"}, "/safety_factors", issues)) {
    return false;
  }
  read_number(safety, "gravity", "/safety_factors", input.safety.gravity,
              issues);
  read_number(safety, "dynamic", "/safety_factors", input.safety.dynamic,
              issues);
  if (!(input.safety.gravity >= 1.0 && input.safety.dynamic >= 1.0)) {
    add_issue(issues, "/safety_factors", "minimum_safety_factor",
              "Safety factors must be at least one.");
  }

  const auto &solver = json["solver"];
  if (!exact_keys(solver,
                  {"max_ik_iterations", "max_optimization_iterations",
                   "step_tolerance"},
                  "/solver", issues)) {
    return false;
  }
  if (!solver["max_ik_iterations"].is_number_integer() ||
      !solver["max_optimization_iterations"].is_number_integer()) {
    add_issue(issues, "/solver", "integer_iteration_limits_required",
              "Solver iteration limits must be integers.");
  } else {
    input.solver.max_ik_iterations = solver["max_ik_iterations"].get<int>();
    input.solver.max_optimization_iterations =
        solver["max_optimization_iterations"].get<int>();
  }
  read_number(solver, "step_tolerance", "/solver", input.solver.step_tolerance,
              issues);
  if (!(input.solver.max_ik_iterations >= 20 &&
        input.solver.max_ik_iterations <= 5000 &&
        input.solver.max_optimization_iterations >= 1 &&
        input.solver.max_optimization_iterations <= 80 &&
        input.solver.step_tolerance > 0.0)) {
    add_issue(issues, "/solver", "invalid_solver_limits",
              "Use 20-5000 IK iterations, 1-80 optimization iterations, and "
              "positive step tolerance.");
  }
  return issues.empty();
}

} // namespace robot_engine::design::internal
