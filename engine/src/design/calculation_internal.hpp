#pragma once

#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <nlohmann/json.hpp>

namespace robot_engine::design::internal {

using Json = nlohmann::json;
using Vector3 = Eigen::Vector3d;
using Vector6 = Eigen::Matrix<double, 6, 1>;
using Vector7 = Eigen::Matrix<double, 7, 1>;
using Matrix67 = Eigen::Matrix<double, 6, 7>;
using Matrix77 = Eigen::Matrix<double, 7, 7>;

inline constexpr std::size_t kJointCount = 7;
inline constexpr std::size_t kDesignedLinkCount = 3;
inline constexpr double kDirectionTolerance = 1e-6;
inline constexpr double kQuaternionTolerance = 1e-6;
inline constexpr double kDamping = 1e-5;
inline constexpr double kOrientationScaleM = 0.25;
inline constexpr double kConditionNumberWarning = 1e4;

struct LinkBound {
  std::string id;
  double minimum{};
  double maximum{};
};

struct JointLimit {
  std::string id;
  double minimum{};
  double maximum{};
};

struct Target {
  std::string id;
  Vector3 position{};
  Eigen::Quaterniond orientation{};
  double position_tolerance{};
  double orientation_tolerance{};
};

struct LinkMass {
  std::string id;
  double mass{};
  double com_fraction{};
  double rotational_inertia{};
};

struct Payload {
  double mass{};
  Vector3 com_tool{};
  Vector3 inertia_diagonal{};
};

struct MotionLimits {
  double linear_velocity{};
  double angular_velocity{};
  double linear_acceleration{};
  double angular_acceleration{};
};

struct EvaluationCase {
  std::string id;
  std::string target_id;
  Vector3 linear_velocity_direction{};
  Vector3 angular_velocity_direction{};
  Vector3 linear_acceleration_direction{};
  Vector3 angular_acceleration_direction{};
};

struct Transmission {
  std::string joint_id;
  double ratio{};
  double efficiency{};
};

struct SafetyFactors {
  double gravity{};
  double dynamic{};
};

struct SolverOptions {
  int max_ik_iterations{};
  int max_optimization_iterations{};
  double step_tolerance{};
};

struct Input {
  std::string revision;
  std::string base_frame;
  std::string tool_frame;
  double base_height{};
  std::array<LinkBound, kDesignedLinkCount> bounds;
  std::array<JointLimit, kJointCount> joint_limits;
  std::vector<Target> targets;
  std::array<LinkMass, kDesignedLinkCount> link_masses;
  Payload payload;
  Vector3 gravity{};
  MotionLimits motion;
  std::vector<EvaluationCase> evaluation_cases;
  std::array<Transmission, kJointCount> transmissions;
  SafetyFactors safety;
  SolverOptions solver;
};

struct Geometry {
  double base_height{};
  std::array<double, kDesignedLinkCount> links{};
};

struct KinematicState {
  Vector3 tool_position{};
  Eigen::Matrix3d tool_rotation{Eigen::Matrix3d::Identity()};
  std::array<Vector3, kJointCount> origins;
  std::array<Vector3, kJointCount> axes;
};

struct IkResult {
  bool converged{};
  Vector7 joints{Vector7::Zero()};
  double position_residual{std::numeric_limits<double>::infinity()};
  double orientation_residual{std::numeric_limits<double>::infinity()};
  int iterations{};
  std::string termination{"iteration_limit"};
};

struct Verification {
  bool all_reachable{};
  std::vector<IkResult> results;
};

enum class LinkLengthSolveStatus {
  converged,
  maximum_geometry_unreachable,
  iteration_limit,
  final_verification_failed,
};

struct LinkLengthSolveResult {
  LinkLengthSolveStatus status{LinkLengthSolveStatus::iteration_limit};
  Geometry geometry;
  Verification verification;
  double scale{1.0};
  double lower_bound{};
  double upper_bound{1.0};
  int iterations{};
};

void add_issue(Json &issues, const std::string &path, const std::string &code,
               const std::string &message);
[[nodiscard]] bool parse_input(const Json &json, Input &input, Json &issues);

[[nodiscard]] Geometry interpolate_geometry(const Input &input, double scale);
[[nodiscard]] bool provably_outside_max_reach(const Input &input);
[[nodiscard]] LinkLengthSolveResult solve_link_lengths(const Input &input);

[[nodiscard]] KinematicState forward(const Geometry &geometry,
                                     const Vector7 &joints);
[[nodiscard]] Matrix67 tool_jacobian(const KinematicState &state,
                                     const Vector3 &point);
[[nodiscard]] Verification verify_geometry(const Input &input,
                                           const Geometry &geometry);

[[nodiscard]] Json calculate_dynamics(const Input &input,
                                      const Geometry &geometry,
                                      const Verification &verification,
                                      Json &warnings);
void append_link_mass_provenance(Json &dynamics, const Input &input,
                                 const Geometry &geometry);

[[nodiscard]] inline std::vector<double> json_vector(const Vector7 &vector) {
  return {vector[0], vector[1], vector[2], vector[3],
          vector[4], vector[5], vector[6]};
}

[[nodiscard]] inline std::vector<double> json_vector(const Vector3 &vector) {
  return {vector.x(), vector.y(), vector.z()};
}

} // namespace robot_engine::design::internal
