# M1 — Minimum Executable Calculation

Status: Planned

Required gate: [G1 — Calculation Proof](../gates/G1-calculation-proof.md)

## Outcome

One executable calculation derives link lengths from workspace requirements and
then derives per-joint requirements from the resulting geometry, payload, and
maximum TCP velocity and acceleration.

## In scope

- Versioned minimum input and result contract with explicit units and frames
- Bounded link-length optimization for a parametric seven-axis arm
- FK/IK verification of requested target points or workspace regions
- Explicit feasible, infeasible, invalid, and non-converged outcomes
- Payload and link mass-property model
- Joint velocity, acceleration, gravity/inertial/peak torque, and power results
- Declared postures or trajectory, transmissions, and safety factors
- Independent references and a single-command demonstration

## Out of scope

- Project persistence and candidate comparison
- Component recommendations
- AI, 3D preview, and desktop packaging

## Deliverables

- Versioned M1 input and output schemas
- Feasible and infeasible reference fixtures
- C++ solver and direct CLI or engine entry point
- Golden, analytic, error-path, and reproducibility tests
- Documented command that runs the complete calculation chain

## Demonstration

Run one input through workspace-to-link-length solving and joint-requirement
calculation without a desktop app, then inspect assumptions, residuals, limiting
cases, warnings, and per-joint results.

## Completion criteria

- [ ] Link-length output feeds dynamics without manual re-entry.
- [ ] Requested workspace coverage is verified rather than inferred from reach.
- [ ] Every dynamic result is traceable to mass properties and an evaluation
  posture or trajectory.
- [ ] Independent reference checks pass within documented tolerances.
- [ ] Invalid, under-specified, non-finite, and unconverged cases cannot succeed.
- [ ] Results record input revision, solver version, units, frames, assumptions,
  residuals, warnings, and validation status.
- [ ] G1 passes.
