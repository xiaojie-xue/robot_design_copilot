#pragma once

#include <array>
#include <cstddef>

namespace robot_engine::kinematics {

inline constexpr std::size_t kReferenceArmJointCount = 7;

struct Pose {
  std::array<double, 3> translation_m;
  // Quaternion component order is explicit at the protocol boundary.
  std::array<double, 4> orientation_xyzw;
};

// Computes base_T_tool0 for the deterministic M0 reference arm. Joint inputs
// use radians and must all be finite.
[[nodiscard]] Pose forward_reference_arm(
    const std::array<double, kReferenceArmJointCount> &joint_positions_rad);

} // namespace robot_engine::kinematics
