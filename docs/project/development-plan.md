# Development Plan

This plan converts the product roadmap into an implementation sequence. The
milestone documents define stable scope and acceptance criteria; this document
defines the delivery order, dependencies, and near-term work.

## Delivery principles

1. Deliver runnable vertical slices and keep `main` buildable.
2. Treat gate evidence as a product artifact, not as work added after a
   milestone.
3. Keep the TypeScript, Rust, and C++ boundaries defined in ADR-0001.
4. Keep engineering algorithms in C++ and require independent validation before
   their results can drive recommendations.
5. Keep accepted requirements in versioned `DesignSpec` revisions. AI can only
   propose reviewable patches.

## Milestone sequence

| Phase | Milestones | Primary result | Starts when | Exit evidence |
| --- | --- | --- | --- | --- |
| Platform proof | M0 | Packaged TypeScript-Rust-C++ path and isolated engine process | Immediately | G0 evidence on all three platforms |
| Product foundation | M1-M2 | Durable projects and stable `DesignSpec v1` | G0 passes | Persistence tests and G1 |
| Engineering proof | M3 | Validated seven-axis calculations | G1 passes | Golden/property tests and G2 |
| Design workflow | M4-M5 | Comparable candidates and traceable component choices | G2 passes for recommendation inputs | Reproducible analyses and G3 |
| Assisted workflow | M6-M7 | Reviewable AI refinement and traceable exports | Stable product data model | Provider safety and export round trips |
| Beta | M8 | Installable cross-platform application | G0-G3 pass | G4 and post-install E2E |

M1 planning may overlap late M0 work, but implementation that depends on an
unproven packaging path must not move ahead of G0. M2 schema exploration may
also start early, but the contract is not accepted until G1.

## M0 implementation plan

M0 is split into small increments so failures are isolated and every increment
has an automated check.

### M0.1 — Repository and transport baseline (in progress)

- Establish CMake/C++20 targets and test layout.
- Publish the protocol-v1 envelope schema and framing rules.
- Implement bounded, length-prefixed frame I/O without JSON semantics.
- Add framing tests for normal, multi-frame, truncated, empty, oversized, and
  write-failure paths.
- Add a transport echo executable for process/pipe smoke testing.
- Add a three-platform CI compile-and-test matrix.

Done when the C++ transport tests pass locally and in the CI matrix. This is a
transport proof only; it is not the M0 end-to-end FK demonstration.

Current evidence: the CMake build, framing tests, protocol tests, sanitizer
build, and a real stdin/stdout health request pass on macOS. The CI matrix is
defined for both the C++ transport and Rust lifecycle client but has not yet
produced review evidence, so M0.1 remains in progress.

### M0.2 — Engineering dependency spike

- Pin Eigen, Pinocchio, Ceres, JSON, and test framework versions.
- Prove clean builds on Windows, macOS, and Linux.
- Record native dependency licenses and packaging behavior.
- Add a minimal seven-axis reference model load.

Done when dependency builds and artifact inspection pass on all target runners.

Current progress: Conan 2.30.0 resolves a locked graph containing Pinocchio
3.8.0, Ceres 2.2.0, Eigen 3.4.1, and nlohmann/json 3.12.0 into the repository's
`build/` directory. The robotics build links all four dependencies;
`engine.health` reports their compiled versions and runs no-output Eigen/Ceres
packaging checks. A deterministic seven-axis Pinocchio model and analytic local
tests are implemented. Windows and Linux package/build evidence remains open,
so M0.2 is not complete.

### M0.3 — Engine protocol and process lifecycle

- Implement request dispatch, request IDs, structured errors, progress, and
  protocol-version rejection in C++.
- Implement Rust child-process startup, matching, timeout, cancellation, crash
  detection, restart, and shutdown.
- Verify stdout contains frames only and stderr contains logs only.
- Add malformed-frame, timeout, cancellation, crash, and orphan-process tests.

Done when lifecycle and contract tests pass on all target runners.

Current progress: the C++ sidecar validates protocol-v1 request envelopes,
correlates responses, rejects unsupported versions and unknown fields, returns
structured errors, and implements `engine.health`. Its request session runs
independent jobs concurrently, serializes complete stdout envelopes, rejects
duplicate in-flight IDs, emits schema-valid progress, propagates cooperative
stop tokens, completes cancelled work with `request_cancelled`, and joins all
workers during shutdown. In-process tests cover out-of-order correlation,
progress ordering, cancellation, duplicate IDs, invalid cancellation, and output
failure. The Rust lifecycle client owns child startup, concurrent request
matching, timeout-triggered cancellation, crash isolation, explicit restart,
graceful/forced shutdown, bounded stderr capture, and strict inbound envelope
validation. Pure protocol, C++ session, fake-process lifecycle, and real
Rust-to-C++ integration tests are implemented. Cross-platform CI review evidence
remains open.

### M0.4 — Desktop FK slice and G0 review

- Scaffold the Tauri 2 and React application.
- Enter seven joint positions in the UI and submit an FK request.
- Compute the pose through Pinocchio and render the versioned result.
- Bundle the engine and native libraries into unsigned artifacts.
- Collect all evidence listed in G0 and conduct the gate review.

Done when G0 passes; only then is M0 marked complete.

Current progress: the C++ sidecar accepts seven finite joint positions through
`kinematics.forward`, computes `base_T_tool0` with Pinocchio, and returns metres
plus an explicit xyzw quaternion. Committed fixtures, analytic zero/quarter-turn
cases, invalid-parameter cases, and real framed process exchanges pass locally.
The Tauri 2/React scaffold now provides seven validated joint inputs, engine
health/restart controls, structured command errors, and versioned pose output.
Tauri owns the Rust lifecycle client behind typed commands; the webview has no
shell capability. The C++ executable can be staged with Tauri's target-triple
sidecar filename, and frontend type checks, domain tests, and production builds
run without starting that sidecar. Desktop compilation, end-to-end execution,
native-library bundling, unsigned three-platform artifacts, and the complete G0
evidence package remain open.

## Next milestones

After G0, execute the remaining work in roadmap order:

1. M1: project format, atomic save/recovery, SQLite migrations, settings,
   keychain, diagnostics, and the base application layout.
2. M2: `DesignSpec v1`, valid/invalid examples, generated boundary types,
   revisions/diffs, unit/frame enforcement, and the parameter editor.
3. M3: parametric arm, FK/Jacobian/IK/dynamics/collision, structured failures,
   golden cases, properties, and platform comparisons.
4. M4: candidate persistence, workspace jobs, stale-result propagation,
   comparison, and a Three.js scene sharing engine frame conventions.
5. M5: versioned catalogs, provenance, hard filtering, compatibility and margin
   checks, explainable ranking, and a manually reviewed reference joint.
6. M6: provider abstraction, streaming/cancellation, structured patches,
   per-field approval, grounded explanations, permissions, and redaction.
7. M7: versioned JSON/BOM/report/URDF/glTF exports, overwrite protection, and
   import/round-trip validation.
8. M8: installers, signing, update policy, sample project, diagnostics, SBOM,
   offline workflow, and clean-system E2E.

## Current constraints and risks

| Risk | Current response |
| --- | --- |
| Native robotics dependencies differ by platform | Resolve in M0.2 before product feature work |
| IPC ambiguity causes cross-language drift | Versioned schema, fixtures, and contract tests |
| Native crash or cancellation leaves orphan processes | Rust owns lifecycle; test crash and shutdown paths in M0.3 |
| Numerical results look plausible but are wrong | G2 requires analytic, independent, golden, property, and platform evidence |
| Catalog gaps appear safe | Missing values are explicit and fail closed |
| AI silently changes requirements | AI emits patches; users approve fields before a revision exists |

The current macOS arm64 environment compiles application targets with Apple
Clang 21. Conan uses a checked-in Apple Clang 17 compatibility profile for the
available libc++ package graph, with all caches and build outputs under
`build/`. Pinocchio 3.8.0, Ceres 2.2.0, Eigen 3.4.1, and nlohmann/json 3.12.0
are installed and locked. The Rust lifecycle client and Tauri/React desktop
scaffold are implemented. Complete M0 work still requires cross-platform
lifecycle evidence, desktop compilation and end-to-end validation, packaged
three-platform evidence, and the remaining M0.4/G0 acceptance paths.
