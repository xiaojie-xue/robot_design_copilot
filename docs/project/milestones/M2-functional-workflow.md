# M2 — Functional Design Workflow

Status: Planned

Required gate: [G2 — Functional Trust](../gates/G2-functional-trust.md)

## Outcome

The validated M1 calculation becomes a complete deterministic seven-axis arm
design workflow that remains usable without AI or a packaged desktop app.

## In scope

- Versioned design, candidate, analysis, and project contracts
- Persistence, revision history, stale-result tracking, and comparison
- FK, Jacobian, IK, workspace, singularity, collision, and trajectory analysis
- Traceable component catalogs, hard filtering, compatibility, margins, ranking
- JSON, BOM, report, URDF, and glTF export

## Out of scope

- AI-assisted changes
- Interactive 3D preview
- Installers and signing

## Deliverables

- Deterministic engineering and project workflow
- Candidate comparison and component-selection entry points
- Versioned exports and round-trip tests
- Golden, property, contract, persistence, and recommendation tests

## Demonstration

Create a requirement set, generate and compare candidates, run analyses, select
traceable components, save and reopen the project, and export the result.

## Completion criteria

- [ ] Every public calculation has a validation case.
- [ ] Input revisions and stale results are traceable.
- [ ] Hard constraint failures cannot be overridden by ranking.
- [ ] Missing catalog data fails closed.
- [ ] Projects and exports pass round-trip tests.
- [ ] G2 passes.
