# M5 — Explainable Component Recommendations

Status: Planned  
Required gate: [G3 — Recommendation Trust](../gates/G3-recommendation-trust.md)

## Outcome

The system recommends traceable motor, reducer, drive, and brake combinations from verified requirements.

## In scope

- Raw source records and normalized catalogs
- Units, source dates, and missing-value handling
- Hard-constraint filtering
- Compatible combination generation
- Continuous, peak, and basic thermal checks
- Margins, ranking factors, and rejection reasons
- One manually reviewed joint-selection case

## Out of scope

- Purchasing automation
- Complete vendor coverage
- Certification-grade bearing life analysis
- Opaque aggregate scores

## Deliverables

- Versioned catalogs and provenance records
- Catalog validation tools
- Recommendation and rejection result models
- Recommendation UI
- Reviewed reference case

## Demonstration

Generate component combinations for one reference joint, inspect constraints, margins, sources, and rejection reasons, then reproduce the result with the same versions.

## Completion criteria

- [ ] Every selected critical parameter has a source record.
- [ ] Missing data is never treated as zero or safe.
- [ ] Hard constraints, margins, and ranking factors are displayed separately.
- [ ] Identical versioned inputs reproduce the result.
- [ ] One complete case passes manual engineering review.
- [ ] G3 passes.
