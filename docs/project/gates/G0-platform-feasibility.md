# G0 — Platform Feasibility Gate

Status: Not reviewed  
Applies after: [M0](../milestones/M0-architecture-baseline.md)  
Decision: Pending

## Purpose

Verify that the TypeScript–Rust–C++ application builds, packages, and runs without an external runtime on every supported platform.

## Entry criteria

- M0 implementation is complete.
- The engine links Eigen, Pinocchio, and Ceres.
- Tauri starts, communicates with, and stops the engine.
- The three-platform CI matrix exists.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| All platform builds complete | CI build matrix | Pending |
| Packages contain engine dependencies | Artifact inspection | Pending |
| End-to-end FK request succeeds | C++ framed smoke test passes locally; desktop smoke test pending | Partial |
| Engine crash is isolated from the UI | Rust child-crash lifecycle test implemented; desktop evidence pending | Partial |
| Timeout and cancellation complete | C++ session and Rust lifecycle tests implemented; matrix evidence pending | Partial |
| Exit leaves no child process | Rust graceful/forced shutdown test implemented; desktop evidence pending | Partial |
| Protocol version is enforced | C++ and Rust contract tests implemented; matrix evidence pending | Partial |

## Exit criteria

- [ ] Windows, macOS, and Linux smoke tests pass.
- [ ] No external compiler or runtime is required.
- [ ] IPC framing, errors, progress, cancellation, and version checks pass.
- [ ] Protocol output uses stdout; logs use stderr.
- [ ] Native dependencies and licenses are recorded.
- [ ] No unresolved packaging blocker remains.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
