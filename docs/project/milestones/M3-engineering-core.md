# M3 — Trusted Engineering Core

Status: Planned  
Required gate: [G2 — Engineering Trust](../gates/G2-engineering-trust.md)

## Outcome

A deterministic seven-axis arm engineering core produces validated, versioned results.

## In scope

- Parametric seven-axis model and URDF loading
- Forward kinematics and Jacobians
- Numerical inverse kinematics
- Joint limits and singularity metrics
- Gravity torque and basic inverse dynamics
- Basic self-collision queries
- Structured warnings and failure results
- Golden and property-based tests

## Out of scope

- Trajectory optimization
- Thermal simulation
- Manufacturing-grade CAD
- Component recommendations

## Deliverables

- C++ engineering modules and engine methods
- Golden cases with independent references
- Numerical tolerance specification
- Cross-platform result tests

## Demonstration

Run FK, IK, Jacobian, and gravity-load analyses on the reference arm and inspect values, units, assumptions, warnings, and failure states.

## Completion criteria

- [ ] Every public calculation has a validation case.
- [ ] Jacobians match finite differences within documented tolerances.
- [ ] IK results pass FK round-trip checks.
- [ ] Non-convergence never produces a success result.
- [ ] Results record input revision, engine version, and units.
- [ ] G2 passes.
