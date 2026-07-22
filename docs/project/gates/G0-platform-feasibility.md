# G0 — Platform Feasibility Gate

Status: Not reviewed  
Applies after: [M0](../milestones/M0-architecture-baseline.md)  
Decision: Pending

## Purpose

Verify that the TypeScript–Rust–C++ application builds, packages, and runs without an external runtime on every supported platform.

## Entry criteria

- [ ] M0 implementation is complete. Packaging and full desktop evidence remain open.
- [x] The engine links Eigen, Pinocchio, and Ceres in the robotics-enabled build.
- [ ] Tauri starts, communicates with, and stops the engine. The binding is
  implemented, but the integrated desktop path has not been executed in a
  child-process-capable environment.
- [x] The Windows, macOS, and Linux CI matrix exists for the C++ core and Rust
  lifecycle client.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| All platform builds complete | Matrix is defined; reviewed green desktop/package runs are not available | Pending |
| Packages contain engine dependencies | Artifact inspection | Pending |
| End-to-end FK request succeeds | C++ framed smoke test and desktop implementation exist; integrated desktop smoke test pending | Partial |
| Engine crash is isolated from the UI | Rust child-crash lifecycle test implemented; desktop evidence pending | Partial |
| Timeout and cancellation complete | C++ session tests pass locally and Rust lifecycle tests exist; matrix evidence pending | Partial |
| Exit leaves no child process | Rust graceful/forced shutdown test implemented; desktop evidence pending | Partial |
| Protocol version is enforced | C++ contract tests pass locally and Rust contract tests exist; matrix evidence pending | Partial |

## Exit criteria

- [ ] Windows, macOS, and Linux smoke tests pass.
- [ ] No external compiler or runtime is required.
- [x] IPC framing, errors, progress, cancellation, and version checks pass
  locally; three-platform gate evidence remains pending.
- [x] Protocol output uses stdout and logs use stderr in the implemented CLI
  boundary and local integration coverage.
- [x] Native dependencies and licenses are recorded in the Conan manifest,
  lockfile, and `THIRD_PARTY_LICENSES.md`.
- [ ] No unresolved packaging blocker remains.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
