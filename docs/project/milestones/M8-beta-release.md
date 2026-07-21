# M8 — Cross-platform Beta

Status: Planned  
Required gate: [G4 — Release Readiness](../gates/G4-release-readiness.md)

## Outcome

Non-developers can install the application and complete the seven-axis arm design workflow on Windows, macOS, and Linux.

## In scope

- Native installers for all three platforms
- macOS signing and notarization
- Windows code signing
- Update policy
- First-run guide and sample project
- Post-install end-to-end tests
- Diagnostic export
- SBOM, licenses, and checksums
- Offline engineering workflow

## Out of scope

- Enterprise deployment
- Cloud accounts and collaboration
- Automated safety certification
- Public wheeled dual-arm support

## Deliverables

- Beta installers
- Release workflow
- Signing and publishing documentation
- Install, upgrade, and uninstall tests
- Quick start and known limitations

## Demonstration

On a clean system, install the app, edit the sample design, run analyses, review recommendations, save and reopen the project, and export a report and URDF.

## Completion criteria

- [ ] Post-install E2E passes on all supported platforms.
- [ ] No development runtime is required.
- [ ] Exit and uninstall leave no engine process.
- [ ] Artifacts include signatures, checksums, and SBOMs.
- [ ] Diagnostics contain no credentials.
- [ ] Known limits and safety boundaries are visible.
- [ ] G4 passes.
