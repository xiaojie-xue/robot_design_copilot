# G4 — Release Readiness Gate

Status: Not reviewed  
Applies after: [M8](../milestones/M8-beta-release.md)  
Decision: Pending

## Purpose

Verify that a non-developer can install, use, update, and remove the beta on every supported platform.

## Entry criteria

- Required M0–M8 work is complete.
- G0–G3 have passed.
- Candidate installers are available.
- Quick-start, safety, and limitation documents are ready.

## Required evidence

| Requirement | Evidence | Status |
| --- | --- | --- |
| Clean installation works | Install E2E report | Pending |
| Reference workflow completes | Product E2E report | Pending |
| Engine lifecycle is clean | Process tests | Pending |
| Signing and notarization verify | Platform verification | Pending |
| Upgrade and uninstall complete | Lifecycle tests | Pending |
| Diagnostics exclude credentials | Security tests | Pending |
| Licenses and SBOM are complete | Release artifacts | Pending |
| Engineering workflow works offline | Offline E2E test | Pending |

## Exit criteria

- [ ] Post-install E2E passes on every supported platform.
- [ ] No development runtime is required.
- [ ] Exit and uninstall leave no engine process.
- [ ] Packages include valid signatures, checksums, and SBOMs.
- [ ] Logs, crash reports, and diagnostics contain no API keys.
- [ ] Known limitations and safety boundaries are visible.
- [ ] No release-blocking defect remains open.

## Review record

| Date | Commit | Reviewers | Decision | Conditions and evidence |
| --- | --- | --- | --- | --- |
| — | — | — | Pending | — |
