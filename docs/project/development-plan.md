# Development Plan

This plan implements the [project roadmap](roadmap.md). M0 remains the original
architecture baseline. G0 now closes when the architecture, dependency set,
sidecar boundary, lifecycle approach, and three-platform build direction are
accepted. The minimum engineering calculation starts at M1.

## Delivery principles

1. Deliver runnable vertical slices and keep `main` buildable.
2. Keep the TypeScript, Rust, and C++ boundaries defined in ADR-0001.
3. Keep engineering algorithms in C++ and expose them through a versioned,
   automation-friendly interface.
4. Treat units, frames, assumptions, solver status, and validation evidence as
   part of every result.
5. Require independent checks before results drive component selection or AI
   explanations.
6. Keep AI changes reviewable and keep 3D derived from engine-owned state.
7. Finish installers and full desktop product E2E in M5, not G0.

## Milestone sequence

| Phase | Milestone | Primary result | Starts when | Exit evidence |
| --- | --- | --- | --- | --- |
| Architecture baseline | M0 | Accepted TypeScript-Rust-C++ boundaries and viable platform strategy | Immediately | Narrowed G0 architecture/platform evidence |
| Calculation proof | M1 | Workspace-to-link-length and joint-requirement executable | G0 passes | Feasible/infeasible fixtures, independent references, and G1 |
| Functional workflow | M2 | Deterministic project, analysis, recommendation, and export workflow | G1 passes | Contract, engineering, recommendation, persistence, export evidence, and G2 |
| AI assistance | M3 | Reviewable requirement patches and grounded explanations | G2 passes | Provider, approval, grounding, redaction evidence, and G3 |
| 3D preview | M4 | Interactive views derived from engine model and results | G2 passes and model conventions are stable | Model/frame consistency and visual interaction tests |
| Desktop product | M5 | Installable cross-platform application | M1-M4 are complete | Clean-system E2E and G4 |

M4 planning may overlap M3 because both consume stable M2 contracts, but M5 is
the integration and release stage for the complete workflow.

## M0 — Existing architecture baseline

M0 retains its original milestone scope and implementation. Current reusable
evidence includes bounded IPC framing, protocol-v1 validation, structured
errors, request progress and cancellation, C++ process sessions, the Rust
lifecycle client, Pinocchio/Ceres/Eigen integration, forward kinematics, and the
Tauri/React scaffold.

The revised G0 review concentrates on:

- Accepted TypeScript, Rust, and C++ ownership boundaries
- Selected native dependencies and recorded licenses
- Versioned sidecar IPC and process isolation
- Windows, macOS, and Linux toolchain and CI feasibility
- Explicit ownership of remaining bundling and installer risks by M5

G0 does not wait for signed or unsigned installers, complete native bundle
inspection, clean-system install tests, or full desktop workflow E2E.

## Immediate next plan: M1 minimum calculation proof

M1 is one input, two connected calculations, and one structured result. It must
be executable through a CLI or direct engine request without launching Tauri.

### M1.1 — Minimum input and result contract

- Define a versioned schema for workspace targets, orientation coverage, joint
  limits, link-length bounds, payload, TCP velocity and acceleration limits,
  mass properties, gravity, evaluation cases, transmissions, and safety factors.
- Reuse repository SI-unit and coordinate-frame conventions.
- Define success, infeasible, invalid-input, under-specified, and non-converged
  states.
- Include provenance, solver version, residuals, warnings, and validation status.
- Commit feasible and infeasible reference inputs before tuning the solver.

Done when schema and fixture tests reject ambiguous units, missing required
assumptions, invalid bounds, non-finite values, and inconsistent frames.

### M1.2 — Workspace-driven link-length solver

- Parameterize the seven-axis arm using a minimal set of link lengths while
  keeping joint axes and limits explicit.
- Convert workspace requirements into target samples with stable identifiers.
- Use bounded deterministic optimization to propose link lengths.
- Verify the candidate with FK/IK reachability rather than optimizer cost alone.
- Report coverage, position/orientation residuals, active bounds, and unreachable
  targets.

Done when the feasible fixture produces repeatable geometry that passes all
required targets and the infeasible fixture returns an explicit infeasible
result.

### M1.3 — Joint kinematic and dynamic requirements

- Consume M1.2 geometry directly without manual re-entry.
- Build a documented reference motion or conservative evaluation set from the
  requested maximum TCP linear/angular velocity and acceleration.
- Map TCP limits to joint velocity and acceleration and report singular or
  ill-conditioned cases.
- Compute gravity and inertial torque from declared payload and link mass
  properties; retain the torque breakdown before safety factors.
- Report per-joint peak velocity, acceleration, combined torque, mechanical
  power, and optional motor-side requirements for declared transmissions.
- Preserve the posture or trajectory sample responsible for every peak.

Done when every per-joint result is traceable to geometry, declared assumptions,
an evaluation case, and a checked dynamics calculation.

### M1.4 — Independent validation and G1

- Add analytic checks for simplified poses and motions.
- Compare the seven-axis result with an independent reference or second
  implementation.
- Add finite-difference and energy/power consistency checks where applicable.
- Define numerical tolerances with engineering justification.
- Run feasible and infeasible fixtures through one documented command.
- Compare supported developer and CI platforms within approved tolerances.

Done when all M1 completion criteria pass and G1 approves the result for use by
M2.

## Later milestones

### M2 — Functional design workflow

1. Promote the M1 contract into stable `DesignSpec`, `CandidateDesign`, and
   `AnalysisResult` models with revisions and stale-result propagation.
2. Add project persistence and candidate comparison through development entry
   points.
3. Extend FK, Jacobian, IK, workspace, singularity, collision, and trajectory
   analysis with structured failures and validation.
4. Add traceable component catalogs, hard filtering, compatibility, margins,
   explainable ranking, and manual reference review.
5. Add JSON, BOM, report, URDF, and glTF export with round-trip validation.

### M3 — AI assistance

1. Add DeepSeek, OpenAI-compatible, and Ollama provider adapters.
2. Add streaming, cancellation, structured patches, per-field approval, and
   mock-provider tests.
3. Ground explanations in versioned deterministic results.
4. Enforce permissions, credential isolation, and log/export redaction.

### M4 — 3D preview

1. Render the engine-owned robot with matching units, joint order, and frames.
2. Add workspace, target, constraint, singularity, collision, and trajectory
   overlays.
3. Connect candidate selection and comparison without duplicating engineering
   calculations in TypeScript.
4. Add visual regression and model/frame consistency tests.

### M5 — Desktop packaging

1. Integrate M1-M4 into the existing Tauri/React scaffold.
2. Complete production save/recovery, migrations, settings, keychain,
   diagnostics, and process lifecycle behavior.
3. Bundle the C++ engine and native libraries on Windows, macOS, and Linux.
4. Add installers, signing/notarization, update policy, sample project, offline
   workflow, licenses, checksums, and SBOM.
5. Run install, upgrade, uninstall, crash, restart, shutdown, and full workflow
   E2E tests on clean systems.

## Current constraints and risks

| Risk | Current response |
| --- | --- |
| Platform packaging delays core value | G0 confirms architecture; M5 owns finished packaging |
| Workspace requirements do not determine unique geometry | Use bounded optimization, report residuals and active constraints, and avoid claiming global optimality |
| Payload and TCP limits under-specify dynamics | Require or derive explicit mass properties, evaluation cases, gravity, transmissions, and safety factors |
| Reachable endpoints hide poor intermediate behavior | Verify target sets, joint limits, conditioning, and modeled constraints |
| Numerical results look plausible but are wrong | Require analytic, independent, finite-difference, property, and platform evidence |
| Catalog gaps appear safe | Represent missing values explicitly and fail closed |
| AI silently changes requirements | AI emits patches and users approve fields before a revision exists |
| 3D drifts from engineering state | Treat engine model and frames as authoritative and test scene consistency |
