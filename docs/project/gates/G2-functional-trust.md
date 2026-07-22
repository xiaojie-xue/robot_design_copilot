# G2 — Functional Trust Gate

Status: Not reviewed

Applies after: [M2](../milestones/M2-functional-workflow.md)

Decision: Pending

## Purpose

Verify the deterministic engineering workflow, project data, recommendations,
and exports before AI or visualization depends on them.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| Public calculations are validated | Golden and property tests | Pending |
| Project revisions reproduce results | Persistence and determinism tests | Pending |
| Candidate comparisons use matching inputs | Comparison tests | Pending |
| Recommendations enforce hard constraints | Reviewed recommendation case | Pending |
| Catalog data is traceable and fails closed | Provenance and negative tests | Pending |
| Exports preserve model semantics | Round-trip tests | Pending |

## Exit criteria

- [ ] No calculation can report success after invalid or unconverged execution.
- [ ] Results identify inputs, algorithms, catalogs, units, and assumptions.
- [ ] Hard constraints and ranking are separated.
- [ ] One full non-AI workflow passes manual engineering review.
- [ ] No correctness or data-integrity defect remains open.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
