# Project Roadmap

Robot Design Copilot is developed as testable vertical slices. M0 remains the
original architecture and platform baseline. After the narrowed G0 review, the
delivery order becomes computation first, then product functions, AI support,
3D preview, and final desktop packaging.

## Status values

| Status | Definition |
| --- | --- |
| Planned | Scope is defined; work has not started. |
| In progress | Implementation is active. |
| Gate review | Implementation is complete; gate review is pending. |
| Complete | Completion criteria and required gates have passed. |
| Blocked | A documented issue prevents further work. |
| Deferred | Work was postponed by an explicit decision. |

## Milestones

| Milestone | Outcome | Required gate | Status |
| --- | --- | --- | --- |
| [M0](milestones/M0-architecture-baseline.md) | Cross-platform architecture baseline and minimal C++ engine path | [G0](gates/G0-platform-feasibility.md) | In progress |
| [M1](milestones/M1-minimum-calculation.md) | Minimal executable link-length and joint-dynamics calculation | [G1](gates/G1-calculation-proof.md) | Gate review |
| [M2](milestones/M2-functional-workflow.md) | Complete deterministic design workflow and engineering functions | [G2](gates/G2-functional-trust.md) | Planned |
| [M3](milestones/M3-ai-assistance.md) | Reviewable AI-assisted requirement refinement and explanation | [G3](gates/G3-ai-safety.md) | Planned |
| [M4](milestones/M4-3d-preview.md) | Engine-derived interactive 3D preview | — | Planned |
| [M5](milestones/M5-desktop-packaging.md) | Installable Windows, macOS, and Linux desktop product | [G4](gates/G4-release-readiness.md) | Planned |

## M0 and G0 boundary

M0 keeps its original architecture-baseline scope and existing implementation.
G0 now confirms the architecture and supported-platform direction: language
boundaries, C++ dependencies, sidecar IPC, process isolation, build toolchains,
and CI strategy. G0 does not require finished installers, signed packages, or a
complete desktop product workflow; those are M5/G4 outcomes.

## M1 minimum calculation chain

M1 must run without a packaged desktop application. A CLI or direct engine
request accepts one structured input and produces one structured result:

```text
workspace requirements
  -> solve candidate link lengths
  -> verify requested workspace coverage
  -> apply payload and TCP speed/acceleration limits
  -> calculate per-joint kinematic and dynamic requirements
  -> emit evidence, assumptions, warnings, and solver status
```

The first calculation derives feasible link lengths from target points or
workspace regions, orientation requirements, joint limits, and link-length
bounds. The second calculation consumes that geometry and reports per-joint
maximum velocity, acceleration, gravity torque, inertial torque, combined peak
torque, and peak mechanical power.

Payload and TCP motion limits do not uniquely determine dynamics. M1 therefore
also requires explicit payload center of mass and inertia, link mass properties,
gravity, evaluation postures or trajectory, transmission assumptions, and
safety factors. A documented conservative model may be used when the user only
provides payload mass.

G1 passes only when the complete calculation is executable, feasible and
infeasible cases are distinguished, independent reference checks pass, and
invalid, under-specified, non-finite, or unconverged inputs cannot report
success.

## M2 functional workflow

After G1, expand the proven calculation into the non-AI engineering product:

- Versioned `DesignSpec`, `CandidateDesign`, and `AnalysisResult` contracts
- Project persistence, revisions, stale-result tracking, and comparison
- FK, Jacobian, IK, workspace, singularity, collision, and trajectory analyses
- Traceable component catalogs, constraint filtering, margins, and ranking
- JSON, BOM, report, URDF, and glTF exports with round-trip validation

All engineering functions remain usable without AI and without a packaged
desktop application.

## M3 AI assistance

Add provider adapters, streaming, cancellation, missing-requirement questions,
structured specification patches, and explanations grounded in deterministic
results. AI may propose and explain, but users approve every accepted field
change and AI never substitutes its own numbers for engineering calculations.

## M4 3D preview

Add an interactive preview of the selected design, joints, links, workspace,
constraints, and trajectories. The preview consumes the same model, units, and
coordinate frames as the engineering engine and is not a second source of
geometry or calculation truth.

## M5 desktop packaging

Integrate M1-M4 into the existing Tauri/React shell. Complete local application
persistence, process lifecycle, diagnostics, native dependency bundling,
installers, signing, updates, SBOMs, and clean-system end-to-end tests. Existing
M0 desktop and lifecycle work is reused here.

## MVP scope

M0-M5 support one target: a parametric seven-axis robot arm. The MVP excludes
wheeled dual-arm robots, real-time control, ROS runtime integration, cloud
collaboration, manufacturing-grade CAD, safety certification, and unrestricted
AI system access.

## Execution rules

1. Use G0 to confirm architecture and platform feasibility, not final product
   packaging.
2. Complete and validate the M1 calculation chain before expanding engineering
   functionality.
3. Never claim dynamic sizing from payload, speed, and acceleration while
   hiding mass properties, postures, trajectories, or safety factors.
4. Validate engineering calculations before using them for recommendations or
   AI explanations.
5. Require user approval for every AI-generated specification change.
6. Keep 3D derived from the deterministic engine model.
7. Finish cross-platform application packaging in M5.

The implementation sequence and current work breakdown are maintained in the
[development plan](development-plan.md).
