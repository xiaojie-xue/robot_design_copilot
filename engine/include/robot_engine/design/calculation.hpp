#pragma once

#include <nlohmann/json_fwd.hpp>

namespace robot_engine::design {

inline constexpr auto kSolverId = "design_solver";

// Runs the complete workspace-to-geometry-to-joint-requirement chain.
// Calculation outcomes, including invalid input and infeasibility, are returned
// as a structured result and never represented by non-finite values.
[[nodiscard]] nlohmann::json calculate(const nlohmann::json &input);

} // namespace robot_engine::design
