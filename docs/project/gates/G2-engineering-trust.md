# G2 — Engineering Trust Gate

Status: Not reviewed  
Applies after: [M3](../milestones/M3-engineering-core.md)  
Decision: Pending

## Purpose

Verify that engineering results are correct, reproducible, and traceable. Recommendation work cannot use these results until this gate passes.

## Entry criteria

- M3 calculations are implemented.
- Golden cases and independent references are committed.
- Numerical tolerances and known limitations are documented.
- Unit, integration, and platform tests pass.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| FK matches references | Golden tests | Pending |
| Jacobians match finite differences | Numerical tests | Pending |
| IK passes FK round trips | Property and golden tests | Pending |
| Gravity loads match references | Golden tests | Pending |
| Boundary and singular states are detected | Boundary tests | Pending |
| Platforms agree within tolerance | CI comparison | Pending |
| Invalid and unconverged inputs fail | Error-path tests | Pending |
| Results identify inputs and versions | Integration tests | Pending |

## Exit criteria

- [ ] Every public calculation has a validation case.
- [ ] Every critical result has an independent check.
- [ ] Tolerances have documented engineering justification.
- [ ] Non-finite, invalid, and unconverged results cannot report success.
- [ ] Platform differences remain within approved tolerances.
- [ ] Known limitations appear in the UI and exports.
- [ ] No correctness-critical defect remains open.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
