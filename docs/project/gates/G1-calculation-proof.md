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
| Feasible workspace produces verified geometry | `feasible-input.json`, M1 unit and direct CLI tests | Pass locally |
| Infeasible workspace cannot succeed | `infeasible-input.json`, no dynamics in result | Pass locally |
| Link-length result feeds dynamics directly | Geometry provenance and integration assertions | Pass locally |
| Joint results match independent references | Moment-arm torque, twist reconstruction, and power checks | Pass locally |
| Units, frames, and assumptions are explicit | Design input/result schemas and result audit | Pass locally |
| Invalid and unconverged inputs fail | Missing, non-finite, under-specified, and iteration-limit tests | Pass locally |
| Results reproduce across platforms | CI comparison | Pending |

## Exit criteria

- [x] One documented command completes the full calculation.
- [x] Every M1 critical result has an independent local check and justified tolerance.
- [x] Limiting postures or trajectory samples are retained with peak results.
- [x] Under-specified dynamics input cannot report a sizing result.
- [ ] No correctness-critical defect remains open.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
