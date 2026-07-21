#include "robot_engine/dependencies.hpp"

#include <cmath>

#include <Eigen/Core>
#include <nlohmann/json.hpp>

#ifdef ROBOT_ENGINE_WITH_ROBOTICS
#include <ceres/ceres.h>
#include <ceres/version.h>
#include <pinocchio/config.hpp>
#endif

#define ROBOT_STRINGIFY_DETAIL(value) #value
#define ROBOT_STRINGIFY(value) ROBOT_STRINGIFY_DETAIL(value)

namespace robot_engine {
namespace {

constexpr std::string_view kEigenVersion =
    ROBOT_STRINGIFY(EIGEN_WORLD_VERSION) "." ROBOT_STRINGIFY(
        EIGEN_MAJOR_VERSION) "." ROBOT_STRINGIFY(EIGEN_MINOR_VERSION);
constexpr std::string_view kNlohmannJsonVersion =
    ROBOT_STRINGIFY(NLOHMANN_JSON_VERSION_MAJOR) "." ROBOT_STRINGIFY(
        NLOHMANN_JSON_VERSION_MINOR) "." ROBOT_STRINGIFY(NLOHMANN_JSON_VERSION_PATCH);

#ifdef ROBOT_ENGINE_WITH_ROBOTICS
constexpr std::string_view kPinocchioVersion = PINOCCHIO_VERSION;
constexpr std::string_view kCeresVersion = CERES_VERSION_STRING;

struct SmokeResidual {
  template <typename Scalar>
  bool operator()(const Scalar *const value, Scalar *residual) const {
    residual[0] = value[0] - Scalar{2.0};
    return true;
  }
};
#endif

} // namespace

DependencyVersions dependency_versions() noexcept {
  return {
      .eigen = kEigenVersion,
      .nlohmann_json = kNlohmannJsonVersion,
#ifdef ROBOT_ENGINE_WITH_ROBOTICS
      .pinocchio = kPinocchioVersion,
      .ceres = kCeresVersion,
#else
      .pinocchio = {},
      .ceres = {},
#endif
  };
}

bool robotics_dependencies_enabled() noexcept {
#ifdef ROBOT_ENGINE_WITH_ROBOTICS
  return true;
#else
  return false;
#endif
}

bool dependency_smoke_check() noexcept {
  const Eigen::Vector3d vector{1.0, 2.0, 3.0};
  const auto transformed = Eigen::Matrix3d::Identity() * vector;
  const auto eigen_ok =
      transformed[0] == 1.0 && transformed[1] == 2.0 && transformed[2] == 3.0;

#ifdef ROBOT_ENGINE_WITH_ROBOTICS
  try {
    double value = 0.0;
    ceres::Problem problem;
    auto *cost = new ceres::AutoDiffCostFunction<SmokeResidual, 1, 1>(
        new SmokeResidual{});
    problem.AddResidualBlock(cost, nullptr, &value);
    return eigen_ok && problem.NumParameterBlocks() == 1 &&
           problem.NumResidualBlocks() == 1;
  } catch (...) {
    return false;
  }
#else
  return eigen_ok;
#endif
}

} // namespace robot_engine

#undef ROBOT_STRINGIFY
#undef ROBOT_STRINGIFY_DETAIL
