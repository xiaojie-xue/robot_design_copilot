# Project Roadmap

Robot Design Copilot is developed as testable vertical slices. Each milestone must produce a runnable result. Critical milestones must also pass an evidence-based gate review.

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
| [M1](milestones/M1-desktop-foundation.md) | Desktop project lifecycle and local persistence | — | Planned |
| [M2](milestones/M2-design-spec-v1.md) | Versioned `DesignSpec v1` contract | [G1](gates/G1-design-contract.md) | Planned |
| [M3](milestones/M3-engineering-core.md) | Verified deterministic engineering core | [G2](gates/G2-engineering-trust.md) | Planned |
| [M4](milestones/M4-candidate-and-3d.md) | Candidate comparison, workspace analysis, and 3D preview | — | Planned |
| [M5](milestones/M5-component-recommendation.md) | Traceable component recommendations | [G3](gates/G3-recommendation-trust.md) | Planned |
| [M6](milestones/M6-ai-copilot.md) | Reviewable AI-assisted requirement refinement | — | Planned |
| [M7](milestones/M7-export.md) | Reusable and traceable design exports | — | Planned |
| [M8](milestones/M8-beta-release.md) | Installable Windows, macOS, and Linux beta | [G4](gates/G4-release-readiness.md) | Planned |

## MVP scope

M0–M8 support one target: a parametric seven-axis robot arm. The MVP excludes wheeled dual-arm robots, real-time control, ROS runtime integration, cloud collaboration, manufacturing-grade CAD, safety certification, and unrestricted AI system access.

## Execution rules

1. Prove cross-platform packaging before adding product features.
2. Stabilize `DesignSpec` before scaling AI prompts.
3. Validate engineering calculations before using them for recommendations.
4. Require user approval for every AI-generated specification change.
5. Use one reference seven-axis arm project for every milestone demonstration.

The implementation sequence and current work breakdown are maintained in the
[development plan](development-plan.md).
