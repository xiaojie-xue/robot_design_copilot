# G1 — Design Contract Gate

Status: Not reviewed  
Applies after: [M2](../milestones/M2-design-spec-v1.md)  
Decision: Pending

## Purpose

Verify that `DesignSpec v1` is unambiguous and stable enough to connect the UI, Rust application layer, and C++ engine.

## Entry criteria

- The schema and examples are committed.
- The editor creates and updates complete specifications.
- Rust, C++, and TypeScript boundary types exist.
- Unit, frame, and migration rules are documented.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| Valid examples pass | Schema CI | Pending |
| Invalid examples fail | Negative schema tests | Pending |
| Language round trips preserve meaning | Contract tests | Pending |
| Units and frames are explicit | Engineering specifications | Pending |
| Revisions can be restored and compared | Persistence tests | Pending |
| Old versions follow a defined path | Migration tests | Pending |
| Assumptions and unknowns persist | Example-project tests | Pending |

## Exit criteria

- [ ] Every engineering quantity has explicit dimension and unit semantics.
- [ ] World, base, joint, and tool frames follow one convention.
- [ ] Unknown values are distinct from zero values.
- [ ] Unsupported versions fail with a structured error.
- [ ] CI detects unsynchronized generated types.
- [ ] No contract ambiguity blocks engineering implementation.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
