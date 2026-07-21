# M4 — Candidate Designs and 3D Preview

Status: Planned  
Required gate: None

## Outcome

Users can generate, analyze, visualize, and compare candidate designs from one `DesignSpec`.

## In scope

- Parametric link lengths and joint ranges
- Candidate generation and persistence
- Workspace sampling and reachability metrics
- Singularity and self-collision visualization
- Joint, link, and end-effector trajectory preview
- Candidate comparison
- Job progress, cancellation, and result caching

## Out of scope

- Full multi-objective optimization
- Manufacturing geometry
- Wheeled dual-arm visualization

## Deliverables

- `CandidateDesign` and `AnalysisResult` models
- Workspace analysis methods
- Three.js robot scene
- Candidate comparison UI
- Cancellation and cache tests

## Demonstration

Generate multiple candidates, compute their workspace and loads, inspect them in 3D, compare metrics, and select one candidate.

## Completion criteria

- [ ] Long-running analyses can be cancelled without blocking the UI.
- [ ] The 3D scene uses the same model and frame conventions as the engine.
- [ ] Each result references a `DesignSpec` revision.
- [ ] Input changes mark dependent results as stale.
- [ ] Saved candidates reproduce the same model.
