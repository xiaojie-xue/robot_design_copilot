#pragma once

#include <string_view>

namespace robot_engine {

struct DependencyVersions {
  std::string_view eigen;
  std::string_view nlohmann_json;
  std::string_view pinocchio;
  std::string_view ceres;
};

[[nodiscard]] DependencyVersions dependency_versions() noexcept;

[[nodiscard]] bool robotics_dependencies_enabled() noexcept;

// A small runtime check that instantiates the linked numerical dependencies.
// This is packaging evidence, not a public engineering calculation.
[[nodiscard]] bool dependency_smoke_check() noexcept;

} // namespace robot_engine
