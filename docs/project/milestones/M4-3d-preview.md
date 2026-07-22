# M4 — 3D Preview

Status: Planned

Required gate: None

## Outcome

Users can interactively inspect the selected robot, workspace, constraints, and
trajectories using the engine-owned model and coordinate conventions.

## In scope

- Joints, links, end effector, targets, and trajectories
- Workspace, singularity, joint-limit, and collision overlays
- Candidate selection and comparison views
- Engine-to-scene model, unit, joint-order, and frame consistency tests

## Out of scope

- Manufacturing-grade CAD
- Engineering calculations implemented in TypeScript
- Desktop installers

## Deliverables

- Three.js preview and overlays
- Selection and comparison interactions
- Visual regression and model/frame consistency tests

## Demonstration

Open a calculated candidate, inspect its reachable workspace and constraints,
play a trajectory, and switch candidates without changing engineering results.

## Completion criteria

- [ ] The scene consumes the same geometry and joint state as the engine.
- [ ] Units, joint order, and frames match documented conventions.
- [ ] Input changes mark dependent previews and analyses stale.
- [ ] The preview never becomes a second calculation source.
