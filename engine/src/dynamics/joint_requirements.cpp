#include "design/calculation_internal.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <utility>

namespace robot_engine::design::internal {
namespace {
struct PointKinematics {
  Vector3 point{};
  Eigen::Matrix<double, 3, 7> linear_jacobian{
      Eigen::Matrix<double, 3, 7>::Zero()};
  Eigen::Matrix<double, 3, 7> angular_jacobian{
      Eigen::Matrix<double, 3, 7>::Zero()};
};

[[nodiscard]] PointKinematics point_on_link(const Geometry &geometry,
                                            const Vector7 &joints,
                                            const std::size_t link_index,
                                            const double fraction) {
  const auto state = forward(geometry, joints);
  Eigen::Matrix3d rotation = Eigen::Matrix3d::Identity();
  Vector3 position{0.0, 0.0, geometry.base_height};
  constexpr std::array<int, kJointCount> axes{2, 1, 2, 1, 2, 1, 2};
  std::size_t current_link = 0;
  std::size_t affected_joints = 0;
  Vector3 point = position;
  for (std::size_t joint = 0; joint < kJointCount; ++joint) {
    Vector3 local_axis = Vector3::Zero();
    local_axis[axes[joint]] = 1.0;
    rotation =
        rotation *
        Eigen::AngleAxisd(joints[static_cast<Eigen::Index>(joint)], local_axis)
            .toRotationMatrix();
    if (joint == 1 || joint == 3 || joint == 5) {
      if (current_link == link_index) {
        point = position +
                rotation *
                    Vector3{geometry.links[current_link] * fraction, 0.0, 0.0};
        affected_joints = joint + 1;
        break;
      }
      position += rotation * Vector3{geometry.links[current_link], 0.0, 0.0};
      ++current_link;
    }
  }
  PointKinematics result{.point = point};
  for (std::size_t joint = 0; joint < affected_joints; ++joint) {
    result.linear_jacobian.col(static_cast<Eigen::Index>(joint)) =
        state.axes[joint].cross(point - state.origins[joint]);
    result.angular_jacobian.col(static_cast<Eigen::Index>(joint)) =
        state.axes[joint];
  }
  return result;
}

[[nodiscard]] PointKinematics payload_kinematics(const Geometry &geometry,
                                                 const Vector7 &joints,
                                                 const Vector3 &com_tool) {
  const auto state = forward(geometry, joints);
  PointKinematics result;
  result.point = state.tool_position + state.tool_rotation * com_tool;
  for (std::size_t joint = 0; joint < kJointCount; ++joint) {
    result.linear_jacobian.col(static_cast<Eigen::Index>(joint)) =
        state.axes[joint].cross(result.point - state.origins[joint]);
    result.angular_jacobian.col(static_cast<Eigen::Index>(joint)) =
        state.axes[joint];
  }
  return result;
}

[[nodiscard]] Vector7 damped_joint_mapping(const Matrix67 &jacobian,
                                           const Vector6 &cartesian) {
  const Eigen::Matrix<double, 6, 6> normal =
      jacobian * jacobian.transpose() +
      kDamping * kDamping * Eigen::Matrix<double, 6, 6>::Identity();
  return jacobian.transpose() * normal.ldlt().solve(cartesian);
}

[[nodiscard]] double condition_number(const Matrix67 &jacobian) {
  const Eigen::JacobiSVD<Matrix67> decomposition(jacobian);
  const auto singular_values = decomposition.singularValues();
  const auto largest = singular_values[0];
  const auto smallest = singular_values[singular_values.size() - 1];
  if (!(smallest > 1e-12)) {
    return std::numeric_limits<double>::infinity();
  }
  return largest / smallest;
}

} // namespace

[[nodiscard]] Json calculate_dynamics(const Input &input,
                                      const Geometry &geometry,
                                      const Verification &verification,
                                      Json &warnings) {
  std::map<std::string, Vector7> target_joints;
  for (std::size_t index = 0; index < input.targets.size(); ++index) {
    target_joints.emplace(input.targets[index].id,
                          verification.results[index].joints);
  }

  struct Peak {
    double velocity{};
    double acceleration{};
    double gravity{};
    double inertial{};
    double combined{};
    double power{};
    std::string velocity_case;
    std::string acceleration_case;
    std::string gravity_case;
    std::string inertial_case;
    std::string torque_case;
    std::string power_case;
  };
  std::array<Peak, kJointCount> peaks;
  Json case_results = Json::array();

  for (const auto &evaluation : input.evaluation_cases) {
    const auto &joints = target_joints.at(evaluation.target_id);
    const auto state = forward(geometry, joints);
    const auto jacobian = tool_jacobian(state, state.tool_position);
    Vector6 tcp_velocity;
    tcp_velocity.head<3>() =
        input.motion.linear_velocity * evaluation.linear_velocity_direction;
    tcp_velocity.tail<3>() =
        input.motion.angular_velocity * evaluation.angular_velocity_direction;
    Vector6 tcp_acceleration;
    tcp_acceleration.head<3>() = input.motion.linear_acceleration *
                                 evaluation.linear_acceleration_direction;
    tcp_acceleration.tail<3>() = input.motion.angular_acceleration *
                                 evaluation.angular_acceleration_direction;
    const Vector7 joint_velocity = damped_joint_mapping(jacobian, tcp_velocity);
    const Vector7 joint_acceleration =
        damped_joint_mapping(jacobian, tcp_acceleration);

    Matrix77 mass_matrix = Matrix77::Zero();
    Vector7 gravity_torque = Vector7::Zero();
    for (std::size_t link = 0; link < kDesignedLinkCount; ++link) {
      const auto point = point_on_link(geometry, joints, link,
                                       input.link_masses[link].com_fraction);
      const auto &properties = input.link_masses[link];
      mass_matrix += properties.mass * point.linear_jacobian.transpose() *
                         point.linear_jacobian +
                     properties.rotational_inertia *
                         point.angular_jacobian.transpose() *
                         point.angular_jacobian;
      gravity_torque -=
          point.linear_jacobian.transpose() * (properties.mass * input.gravity);
    }
    const auto payload =
        payload_kinematics(geometry, joints, input.payload.com_tool);
    const auto payload_rotational_inertia =
        input.payload.inertia_diagonal.mean();
    mass_matrix += input.payload.mass * payload.linear_jacobian.transpose() *
                       payload.linear_jacobian +
                   payload_rotational_inertia *
                       payload.angular_jacobian.transpose() *
                       payload.angular_jacobian;
    gravity_torque -= payload.linear_jacobian.transpose() *
                      (input.payload.mass * input.gravity);
    const Vector7 inertial_torque = mass_matrix * joint_acceleration;
    const Vector7 conservative_torque =
        input.safety.gravity * gravity_torque.cwiseAbs() +
        input.safety.dynamic * inertial_torque.cwiseAbs();
    const Vector7 mechanical_power =
        conservative_torque.cwiseProduct(joint_velocity.cwiseAbs());
    const auto conditioning = condition_number(jacobian);
    if (!std::isfinite(conditioning) ||
        conditioning > kConditionNumberWarning) {
      warnings.push_back({
          {"code", "ill_conditioned_evaluation_case"},
          {"case_id", evaluation.id},
          {"message",
           "The TCP-to-joint mapping is singular or ill-conditioned; reported "
           "damped least-squares requirements need trajectory review."},
      });
    }
    case_results.push_back({
        {"case_id", evaluation.id},
        {"target_id", evaluation.target_id},
        {"joint_positions_rad", json_vector(joints)},
        {"joint_velocities_rad_s", json_vector(joint_velocity)},
        {"joint_accelerations_rad_s2", json_vector(joint_acceleration)},
        {"gravity_torque_nm", json_vector(gravity_torque)},
        {"inertial_torque_nm", json_vector(inertial_torque)},
        {"required_torque_nm", json_vector(conservative_torque)},
        {"mechanical_power_w", json_vector(mechanical_power)},
        {"jacobian_condition_number",
         std::isfinite(conditioning) ? Json(conditioning) : Json(nullptr)},
        {"mapping", "damped_least_squares"},
    });

    for (std::size_t joint = 0; joint < kJointCount; ++joint) {
      auto &peak = peaks[joint];
      const auto index = static_cast<Eigen::Index>(joint);
      const auto update = [&](double value, double &stored,
                              std::string &case_id) {
        value = std::abs(value);
        if (value >= stored) {
          stored = value;
          case_id = evaluation.id;
        }
      };
      update(joint_velocity[index], peak.velocity, peak.velocity_case);
      update(joint_acceleration[index], peak.acceleration,
             peak.acceleration_case);
      update(gravity_torque[index], peak.gravity, peak.gravity_case);
      update(inertial_torque[index], peak.inertial, peak.inertial_case);
      update(conservative_torque[index], peak.combined, peak.torque_case);
      update(mechanical_power[index], peak.power, peak.power_case);
    }
  }

  Json requirements = Json::array();
  for (std::size_t joint = 0; joint < kJointCount; ++joint) {
    const auto &peak = peaks[joint];
    const auto &transmission = input.transmissions[joint];
    requirements.push_back({
        {"joint_id", "joint_" + std::to_string(joint + 1)},
        {"peak_velocity_rad_s",
         {{"value", peak.velocity}, {"case_id", peak.velocity_case}}},
        {"peak_acceleration_rad_s2",
         {{"value", peak.acceleration}, {"case_id", peak.acceleration_case}}},
        {"peak_gravity_torque_nm",
         {{"value", peak.gravity}, {"case_id", peak.gravity_case}}},
        {"peak_inertial_torque_nm",
         {{"value", peak.inertial}, {"case_id", peak.inertial_case}}},
        {"peak_required_torque_nm",
         {{"value", peak.combined}, {"case_id", peak.torque_case}}},
        {"peak_mechanical_power_w",
         {{"value", peak.power}, {"case_id", peak.power_case}}},
        {"motor_side",
         {{"peak_speed_rad_s", peak.velocity * transmission.ratio},
          {"peak_torque_nm",
           peak.combined / (transmission.ratio * transmission.efficiency)},
          {"peak_power_w", peak.power / transmission.efficiency},
          {"transmission_ratio", transmission.ratio},
          {"transmission_efficiency", transmission.efficiency}}},
    });
  }
  return {
      {"model", "point-mass Jacobian mass matrix with declared scalar link "
                "inertias and payload diagonal inertia"},
      {"motion_mapping", "damped least-squares at declared target postures"},
      {"mapping_parameters",
       {{"damping", kDamping},
        {"orientation_scale_m_per_rad", kOrientationScaleM},
        {"condition_number_warning_threshold", kConditionNumberWarning}}},
      {"safety_factors",
       {{"gravity", input.safety.gravity}, {"dynamic", input.safety.dynamic}}},
      {"mass_properties",
       {{"links", Json::array()},
        {"payload",
         {{"mass_kg", input.payload.mass},
          {"com_tool_m", json_vector(input.payload.com_tool)},
          {"inertia_diagonal_kg_m2",
           json_vector(input.payload.inertia_diagonal)}}}}},
      {"evaluation_cases", std::move(case_results)},
      {"joint_requirements", std::move(requirements)},
  };
}

void append_link_mass_provenance(Json &dynamics, const Input &input,
                                 const Geometry &geometry) {
  auto &links = dynamics["mass_properties"]["links"];
  for (std::size_t index = 0; index < kDesignedLinkCount; ++index) {
    links.push_back({
        {"id", input.link_masses[index].id},
        {"solved_length_m", geometry.links[index]},
        {"mass_kg", input.link_masses[index].mass},
        {"com_fraction", input.link_masses[index].com_fraction},
        {"rotational_inertia_kg_m2",
         input.link_masses[index].rotational_inertia},
    });
  }
}

} // namespace robot_engine::design::internal
