# M0 — Architecture Baseline

Status: In progress
Required gate: [G0 — Platform Feasibility](../gates/G0-platform-feasibility.md)

## Outcome

A minimal TypeScript–Rust–C++ path builds and runs on Windows, macOS, and Linux.

## In scope

- [x] React/Tauri desktop shell
- [x] C++20 core and engine executable
- [x] Minimal Eigen, Pinocchio, and Ceres integration
- [x] Length-prefixed stdin/stdout IPC
- [x] Request IDs, errors, progress, cancellation, and protocol versioning
- [ ] Three-platform full-application CI smoke builds. The core/lifecycle matrix
  exists, but robotics-enabled desktop build and package jobs remain open.

## Out of scope

- Complete `DesignSpec`
- Product engineering analyses
- Database and AI providers
- Signed installers

## Deliverables

- [x] `robot-engine-core` library
- [x] `robot-engine-cli` sidecar
- [x] Rust `engine-client`
- [x] Forward-kinematics implementation path across React, Tauri, Rust, and C++
- [ ] Recorded integrated desktop FK execution and clean-exit evidence
- [x] Architecture ADRs and build workflow

## Demonstration

- [x] The desktop UI accepts and validates seven joint positions.
- [x] The C++ protocol path computes `base_T_tool0` with Pinocchio.
- [x] The React result view renders versioned translation and xyzw orientation.
- [ ] Run the complete desktop path and close it without leaving an engine
  process; this requires an environment that permits the app to spawn its
  sidecar.

## Completion criteria

- [ ] Unsigned application artifacts run on all three platforms.
- [ ] No external compiler or runtime is required.
- [ ] Engine failure does not terminate the UI process. Process isolation,
  restart, and structured error mapping are implemented; desktop crash-path
  evidence remains pending.
- [x] IPC error, timeout, cancellation, and version paths are tested locally.
- [ ] G0 passes.
