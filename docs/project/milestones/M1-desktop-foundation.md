# M1 — Desktop Foundation

Status: Planned  
Required gate: None

## Outcome

Users can create, save, close, and reopen a local design project.

## In scope

- Project creation, open, save, and recent-project list
- SQLite initialization and migrations
- Application data and project file layout
- Engine startup, health monitoring, restart, and shutdown
- Background job state model
- Logging, diagnostics, settings, and system keychain access

## Out of scope

- Engineering parameter editor
- AI chat
- Engineering analysis and 3D preview

## Deliverables

- Rust application, persistence, platform, and engine-client crates
- Migration and lifecycle tests
- Project format draft
- Base React layout

## Demonstration

Create and save a project, restart the application, recover the project, then simulate an engine crash and restart it from the UI.

## Completion criteria

- [ ] Projects survive a normal restart and an interrupted write test.
- [ ] Database migrations pass automated tests.
- [ ] React has no direct database, filesystem, or engine access.
- [ ] API keys are stored only in the system keychain.
- [ ] Engine shutdown leaves no child process.
