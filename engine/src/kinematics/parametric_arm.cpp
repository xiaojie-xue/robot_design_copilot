#include "design/calculation_internal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

#include <Eigen/Dense>
#include <Eigen/Geometry>

namespace robot_engine::design::internal {

[[nodiscard]] KinematicState forward(const Geometry &geometry,
                                     const Vector7 &joints) {
  KinematicState state;
  Eigen::Matrix3d rotation = Eigen::Matrix3d::Identity();
  Vector3 position{0.0, 0.0, geometry.base_height};
  constexpr std::array<int, kJointCount> axes{2, 1, 2, 1, 2, 1, 2};
  std::size_t link_index = 0;
  for (std::size_t index = 0; index < kJointCount; ++index) {
    Vector3 local_axis = Vector3::Zero();
    local_axis[axes[index]] = 1.0;
    state.origins[index] = position;
    state.axes[index] = rotation * local_axis;
    rotation =
        rotation *
        Eigen::AngleAxisd(joints[static_cast<Eigen::Index>(index)], local_axis)
            .toRotationMatrix();
    if (index == 1 || index == 3 || index == 5) {
      position += rotation * Vector3{geometry.links[link_index], 0.0, 0.0};
      ++link_index;
    }
  }
  state.tool_position = position;
  state.tool_rotation = rotation;
  return state;
}

[[nodiscard]] Matrix67 tool_jacobian(const KinematicState &state,
                                     const Vector3 &point) {
  Matrix67 jacobian;
  for (std::size_t index = 0; index < kJointCount; ++index) {
    jacobian.block<3, 1>(0, static_cast<Eigen::Index>(index)) =
        state.axes[index].cross(point - state.origins[index]);
    jacobian.block<3, 1>(3, static_cast<Eigen::Index>(index)) =
        state.axes[index];
  }
  return jacobian;
}

[[nodiscard]] Vector3 orientation_error(const Eigen::Matrix3d &desired,
                                        const Eigen::Matrix3d &actual) {
  const Eigen::AngleAxisd difference(desired * actual.transpose());
  if (!std::isfinite(difference.angle()) || difference.angle() < 1e-15) {
    return Vector3::Zero();
  }
  return difference.axis() * difference.angle();
}

[[nodiscard]] Vector7 clamp_joints(const Input &input, Vector7 joints) {
  for (std::size_t index = 0; index < kJointCount; ++index) {
    joints[static_cast<Eigen::Index>(index)] = std::clamp(
        joints[static_cast<Eigen::Index>(index)],
        input.joint_limits[index].minimum, input.joint_limits[index].maximum);
  }
  return joints;
}

[[nodiscard]] std::vector<Vector7>
ik_seeds(const Input &input, const Geometry &geometry, const Target &target) {
  std::vector<Vector7> seeds;
  seeds.push_back(clamp_joints(input, Vector7::Zero()));
  for (const double sign : {1.0, -1.0}) {
    Vector7 seed = Vector7::Zero();
    seed[0] = std::atan2(target.position.y(), target.position.x());
    const auto planar_x = std::hypot(target.position.x(), target.position.y());
    const auto planar_z = target.position.z() - geometry.base_height;
    const auto first = geometry.links[0];
    const auto second = geometry.links[1] + geometry.links[2];
    const auto distance_squared = planar_x * planar_x + planar_z * planar_z;
    const auto cosine =
        std::clamp((distance_squared - first * first - second * second) /
                       (2.0 * first * second),
                   -1.0, 1.0);
    seed[3] = sign * std::acos(cosine);
    seed[1] = -std::atan2(planar_z, planar_x) -
              std::atan2(second * std::sin(seed[3]),
                         first + second * std::cos(seed[3]));
    seeds.push_back(clamp_joints(input, seed));
  }
  for (const double sign : {1.0, -1.0}) {
    Vector7 seed = Vector7::Zero();
    seed << 0.0, 0.35 * sign, -0.2 * sign, -0.7 * sign, 0.2 * sign, 0.35 * sign,
        0.0;
    seeds.push_back(clamp_joints(input, seed));
  }
  return seeds;
}

[[nodiscard]] IkResult solve_ik(const Input &input, const Geometry &geometry,
                                const Target &target) {
  IkResult best;
  const auto desired_rotation = target.orientation.toRotationMatrix();
  for (auto joints : ik_seeds(input, geometry, target)) {
    IkResult candidate;
    for (int iteration = 0; iteration < input.solver.max_ik_iterations;
         ++iteration) {
      const auto state = forward(geometry, joints);
      const auto position_error = target.position - state.tool_position;
      const auto rotation_error =
          orientation_error(desired_rotation, state.tool_rotation);
      candidate.position_residual = position_error.norm();
      candidate.orientation_residual = rotation_error.norm();
      candidate.iterations = iteration;
      if (candidate.position_residual <= target.position_tolerance &&
          candidate.orientation_residual <= target.orientation_tolerance) {
        candidate.converged = true;
        candidate.termination = "target_tolerances_satisfied";
        candidate.joints = joints;
        return candidate;
      }

      auto jacobian = tool_jacobian(state, state.tool_position);
      Vector6 error;
      error.head<3>() = position_error;
      error.tail<3>() = kOrientationScaleM * rotation_error;
      jacobian.bottomRows<3>() *= kOrientationScaleM;
      const Eigen::Matrix<double, 6, 6> normal =
          jacobian * jacobian.transpose() +
          kDamping * kDamping * Eigen::Matrix<double, 6, 6>::Identity();
      const Vector7 step = jacobian.transpose() * normal.ldlt().solve(error);
      if (!step.allFinite()) {
        candidate.termination = "non_finite_step";
        break;
      }
      joints = clamp_joints(input, joints + step);
      if (step.norm() <= input.solver.step_tolerance) {
        candidate.termination = "step_tolerance_without_feasibility";
        break;
      }
    }
    candidate.joints = joints;
    const auto candidate_score =
        candidate.position_residual +
        kOrientationScaleM * candidate.orientation_residual;
    const auto best_score =
        best.position_residual + kOrientationScaleM * best.orientation_residual;
    if (candidate_score < best_score) {
      best = candidate;
    }
  }
  return best;
}

[[nodiscard]] Verification verify_geometry(const Input &input,
                                           const Geometry &geometry) {
  Verification verification{.all_reachable = true};
  for (const auto &target : input.targets) {
    auto result = solve_ik(input, geometry, target);
    verification.all_reachable = verification.all_reachable && result.converged;
    verification.results.push_back(std::move(result));
  }
  return verification;
}

} // namespace robot_engine::design::internal
