# M5 — Desktop Packaging

Status: Planned

Required gate: [G4 — Release Readiness](../gates/G4-release-readiness.md)

## Outcome

Non-developers can install and complete the validated M1-M4 workflow on Windows,
macOS, and Linux.

## In scope

- Integration with the existing Tauri/React scaffold
- Production project persistence, settings, keychain, and diagnostics
- C++ engine and native-library bundling
- Installers, signing/notarization, update policy, checksums, and SBOM
- Install, upgrade, uninstall, lifecycle, offline, and full-workflow E2E tests

## Out of scope

- Enterprise deployment and cloud collaboration
- Automated safety certification
- Manufacturing-grade CAD

## Deliverables

- Cross-platform desktop application and release workflow
- Signed release candidates, checksums, licenses, and SBOM
- Quick start, sample project, diagnostics, and known limitations
- Clean-system end-to-end evidence

## Demonstration

Install on a clean system, run the reference design workflow, use AI and 3D,
save and reopen the project, export results, exit cleanly, and uninstall.

## Completion criteria

- [ ] Post-install E2E passes on all supported platforms.
- [ ] No development runtime is required.
- [ ] Exit and uninstall leave no engine process.
- [ ] Artifacts include signatures, checksums, licenses, and SBOM.
- [ ] Diagnostics contain no credentials.
- [ ] G4 passes.
