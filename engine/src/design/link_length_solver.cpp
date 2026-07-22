#include "design/calculation_internal.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace robot_engine::design::internal {

[[nodiscard]] Geometry interpolate_geometry(const Input &input,
                                            const double scale) {
  Geometry geometry{.base_height = input.base_height};
  for (std::size_t index = 0; index < geometry.links.size(); ++index) {
    geometry.links[index] =
        input.bounds[index].minimum +
        scale * (input.bounds[index].maximum - input.bounds[index].minimum);
  }
  return geometry;
}

[[nodiscard]] bool provably_outside_max_reach(const Input &input) {
  double maximum_reach = 0.0;
  for (const auto &bound : input.bounds) {
    maximum_reach += bound.maximum;
  }
  return std::any_of(input.targets.begin(), input.targets.end(),
                     [&](const Target &target) {
                       const Vector3 shoulder{0.0, 0.0, input.base_height};
                       return (target.position - shoulder).norm() >
                              maximum_reach + target.position_tolerance;
                     });
}

[[nodiscard]] LinkLengthSolveResult solve_link_lengths(const Input &input) {
  LinkLengthSolveResult result;
  result.geometry = interpolate_geometry(input, 1.0);
  result.verification = verify_geometry(input, result.geometry);
  if (!result.verification.all_reachable) {
    result.status = LinkLengthSolveStatus::maximum_geometry_unreachable;
    return result;
  }

  for (; result.iterations < input.solver.max_optimization_iterations;
       ++result.iterations) {
    const auto midpoint = 0.5 * (result.lower_bound + result.upper_bound);
    auto verification =
        verify_geometry(input, interpolate_geometry(input, midpoint));
    if (verification.all_reachable) {
      result.upper_bound = midpoint;
      result.verification = std::move(verification);
    } else {
      result.lower_bound = midpoint;
    }
    if (result.upper_bound - result.lower_bound <=
        input.solver.step_tolerance) {
      ++result.iterations;
      break;
    }
  }

  if (result.upper_bound - result.lower_bound > input.solver.step_tolerance) {
    result.status = LinkLengthSolveStatus::iteration_limit;
    return result;
  }

  result.scale = result.upper_bound;
  result.geometry = interpolate_geometry(input, result.scale);
  result.verification = verify_geometry(input, result.geometry);
  result.status = result.verification.all_reachable
                      ? LinkLengthSolveStatus::converged
                      : LinkLengthSolveStatus::final_verification_failed;
  return result;
}

} // namespace robot_engine::design::internal
