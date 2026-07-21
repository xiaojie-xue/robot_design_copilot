# G3 — Recommendation Trust Gate

Status: Not reviewed  
Applies after: [M5](../milestones/M5-component-recommendation.md)  
Decision: Pending

## Purpose

Verify that recommendations use traceable data and verified constraints, and that every selection and rejection is explainable.

## Entry criteria

- Initial catalogs and import pipelines are complete.
- Filtering, combination, and ranking logic is implemented.
- One complete joint-selection case is ready for review.
- G2 has passed.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| Critical parameters have source and date | Provenance audit | Pending |
| Catalog units are normalized | Validation tests | Pending |
| Missing values never imply safety | Negative tests | Pending |
| Hard constraints are separate from ranking | Recommendation tests | Pending |
| Margins can be recomputed | Golden recommendation case | Pending |
| Rejections include a reason | Result-model and UI tests | Pending |
| Versioned inputs reproduce results | Determinism test | Pending |
| Reference case passes manual review | Review record | Pending |

## Exit criteria

- [ ] Every selected critical value has a source record.
- [ ] Continuous, peak, and duty-cycle ratings are distinct.
- [ ] A hard-constraint failure cannot be overridden by ranking.
- [ ] Users can inspect margins, assumptions, and missing data.
- [ ] Results record catalog and algorithm versions.
- [ ] One end-to-end case passes manual engineering review.
- [ ] No data-integrity or margin defect remains open.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
