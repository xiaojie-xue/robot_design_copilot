# G0 — Platform Feasibility Gate

Status: Not reviewed  
Applies after: [M0](../milestones/M0-architecture-baseline.md)  
Decision: Pending

## Purpose

Confirm the TypeScript–Rust–C++ architecture and the Windows, macOS, and Linux
platform direction before starting the minimum design calculation. G0 validates
the foundation and major platform choices; it does not certify finished desktop
packaging.

## Entry criteria

- The React/Tauri, Rust lifecycle, and C++ engine boundaries are implemented.
- The engine links the selected Eigen, Pinocchio, Ceres, and JSON dependencies.
- The sidecar protocol covers framing, request IDs, errors, progress,
  cancellation, and version rejection.
- Windows, macOS, and Linux build toolchains and CI jobs are defined.
- Architecture ADRs and dependency/license records are committed.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| Language and process boundaries are fixed | ADR review and architecture diagram | Partial |
| C++ dependency set resolves and builds | Lockfile, build logs, and health output | Partial |
| Sidecar IPC contract works | Protocol, framing, and real-process smoke tests | Partial |
| Engine lifecycle is isolated | Rust startup, timeout, cancellation, crash, restart, and shutdown tests | Partial |
| All target platforms have a viable build path | Toolchain definitions and CI compile/test matrix | Pending |
| Native dependency and license obligations are known | Lockfile and third-party license inventory | Partial |
| Remaining packaging risks are owned by M5 | Recorded risk review | Pending |

## Exit criteria

- [ ] TypeScript, Rust, and C++ ownership boundaries are accepted.
- [ ] The sidecar protocol and process-isolation approach have no unresolved
  architecture blocker.
- [ ] The selected robotics dependencies have a documented build path for
  Windows, macOS, and Linux.
- [ ] Baseline core and lifecycle compile/test jobs pass on the supported CI
  runners, or an explicitly approved platform exception is recorded.
- [ ] Native dependencies, licenses, and known packaging risks are documented.
- [ ] Remaining installer, signing, bundling, and full desktop E2E work is
  assigned to M5/G4 rather than treated as a G0 blocker.

## Not required for G0

- Finished unsigned or signed installers
- Full native-library bundle inspection
- Clean-system installation, update, or uninstall tests
- Complete desktop engineering workflow E2E
- Final UI crash recovery or product diagnostics

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
