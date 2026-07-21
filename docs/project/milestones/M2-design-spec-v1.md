# M2 — DesignSpec v1

Status: Planned  
Required gate: [G1 — Design Contract](../gates/G1-design-contract.md)

## Outcome

`DesignSpec v1` is the versioned source of truth for seven-axis arm requirements.

## In scope

- Seven-axis arm requirement schema
- SI units and coordinate-frame conventions
- `assumptions`, `unknowns`, and `provenance`
- Revisions, diffs, and migration behavior
- Rust, C++, and TypeScript boundary types
- Parameter editor and cross-field validation
- Valid and invalid example sets

## Out of scope

- Candidate designs and analysis results
- AI-generated changes
- Wheeled dual-arm requirements

## Deliverables

- `schemas/design-spec/v1.schema.json`
- Schema examples and tests
- Unit, frame, and migration specifications
- Parameter editor
- Cross-language contract CI

## Demonstration

Create a complete requirement set, review missing values and assumptions, save a revision, change fields, and compare revisions.

## Completion criteria

- [ ] Every engineering quantity has explicit unit semantics.
- [ ] Frame and quaternion conventions are documented and tested.
- [ ] Unknown values cannot be confused with zero values.
- [ ] Cross-language round trips preserve contract semantics.
- [ ] Schema changes trigger compatibility checks.
- [ ] G1 passes.
