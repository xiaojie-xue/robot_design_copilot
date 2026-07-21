# Third-party Dependencies

This file records dependencies introduced before release packaging. Release
artifacts will include the complete license texts and an SBOM.

| Dependency | Pinned version | Purpose | License | Source |
| --- | --- | --- | --- | --- |
| nlohmann/json | 3.12.0 | Engine protocol JSON parsing and serialization | MIT | <https://github.com/nlohmann/json> |
| Eigen | 3.4.1 | Linear algebra compatible with the selected Pinocchio package | MPL-2.0 / LGPL-3.0-or-later portions | <https://gitlab.com/libeigen/eigen> |
| Pinocchio | 3.8.0 | Robot models, kinematics, and dynamics | BSD-2-Clause | <https://github.com/stack-of-tasks/pinocchio> |
| Ceres Solver | 2.2.0 | Nonlinear optimization | BSD-3-Clause | <https://github.com/ceres-solver/ceres-solver> |

Pinocchio and Ceres are pinned in the Conan manifest. Their targets are linked
by the robotics-enabled build introduced during the M0.2 dependency spike.
