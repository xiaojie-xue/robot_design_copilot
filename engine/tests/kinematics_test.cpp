#include "robot_engine/kinematics/reference_arm.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

using robot_engine::kinematics::forward_reference_arm;
using robot_engine::kinematics::kReferenceArmJointCount;

void require(const bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

[[nodiscard]] bool near(const double actual, const double expected,
                        const double tolerance = 1e-12) {
  return std::abs(actual - expected) <= tolerance;
}

void test_zero_configuration() {
  const auto pose = forward_reference_arm({});
  require(near(pose.translation_m[0], 1.26), "zero pose x");
  require(near(pose.translation_m[1], 0.0), "zero pose y");
  require(near(pose.translation_m[2], 0.54), "zero pose z");
  require(near(pose.orientation_xyzw[0], 0.0), "zero quaternion x");
  require(near(pose.orientation_xyzw[1], 0.0), "zero quaternion y");
  require(near(pose.orientation_xyzw[2], 0.0), "zero quaternion z");
  require(near(pose.orientation_xyzw[3], 1.0), "zero quaternion w");
}

void test_base_quarter_turn() {
  std::array<double, kReferenceArmJointCount> joints{};
  joints[0] = std::acos(-1.0) / 2.0;
  const auto pose = forward_reference_arm(joints);
  const auto half_sqrt_two = std::sqrt(0.5);
  require(near(pose.translation_m[0], 0.0), "quarter-turn pose x");
  require(near(pose.translation_m[1], 1.26), "quarter-turn pose y");
  require(near(pose.translation_m[2], 0.54), "quarter-turn pose z");
  require(near(pose.orientation_xyzw[2], half_sqrt_two),
          "quarter-turn quaternion z");
  require(near(pose.orientation_xyzw[3], half_sqrt_two),
          "quarter-turn quaternion w");
}

void test_non_finite_input() {
  std::array<double, kReferenceArmJointCount> joints{};
  joints[3] = std::numeric_limits<double>::quiet_NaN();
  try {
    static_cast<void>(forward_reference_arm(joints));
  } catch (const std::invalid_argument &) {
    return;
  }
  require(false, "non-finite joint input is rejected");
}

} // namespace

int main() {
  test_zero_configuration();
  test_base_quarter_turn();
  test_non_finite_input();
  std::cout << "All kinematics tests passed\n";
  return 0;
}
