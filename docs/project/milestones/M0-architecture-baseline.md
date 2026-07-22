# M0 — Architecture Baseline

Status: In progress
Required gate: [G0 — Platform Feasibility](../gates/G0-platform-feasibility.md)

## Outcome

A minimal TypeScript–Rust–C++ path builds and runs on Windows, macOS, and Linux.

## In scope

- React/Tauri desktop shell
- C++20 core and engine executable
- Minimal Eigen, Pinocchio, and Ceres integration
- Length-prefixed stdin/stdout IPC
- Request IDs, errors, progress, cancellation, and protocol versioning
- Three-platform CI smoke builds

## Out of scope

- Complete `DesignSpec`
- Product engineering analyses
- Database and AI providers
- Signed installers

## Deliverables

- `robot-engine-core` library
- `robot-engine-cli` sidecar
- Rust `engine-client`
- End-to-end forward-kinematics example
- Architecture ADRs and build workflow

## Demonstration

Enter joint positions in the desktop UI, compute the end-effector pose in Pinocchio, display the result, and close the application without leaving an engine process.

## Completion criteria

- [ ] Unsigned application artifacts run on all three platforms.
- [ ] No external compiler or runtime is required.
- [ ] Engine failure does not terminate the UI process.
- [x] IPC error, timeout, cancellation, and version paths are tested locally.
- [ ] G0 passes.
