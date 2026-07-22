# G1 — Calculation Proof Gate

Status: Not reviewed

Applies after: [M1](../milestones/M1-minimum-calculation.md)

Decision: Pending

## Purpose

Verify that the minimum workspace-to-link-length and joint-requirement chain is
correct, reproducible, traceable, and safe to extend.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| Feasible workspace produces verified geometry | Golden end-to-end fixture | Pending |
| Infeasible workspace cannot succeed | Negative fixture | Pending |
| Link-length result feeds dynamics directly | Integration test | Pending |
| Joint results match independent references | Analytic and independent comparisons | Pending |
| Units, frames, and assumptions are explicit | Schema and result audit | Pending |
| Invalid and unconverged inputs fail | Error-path tests | Pending |
| Results reproduce across platforms | CI comparison | Pending |

## Exit criteria

- [ ] One documented command completes the full calculation.
- [ ] Every critical result has an independent check and justified tolerance.
- [ ] Limiting postures or trajectory samples are retained with peak results.
- [ ] Under-specified dynamics input cannot report a sizing result.
- [ ] No correctness-critical defect remains open.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
