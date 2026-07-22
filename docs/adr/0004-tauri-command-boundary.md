# ADR-0004: Tauri Command Boundary and Bundled Engine

Status: Accepted  
Date: 2026-07-22

## Context

The React webview must request engineering calculations without gaining general
process access. Tauri must also locate the correct engine executable in packaged
Windows, macOS, and Linux applications while preserving Rust ownership of the
child lifecycle defined by ADR-0002.

## Decision

- Expose purpose-built Tauri commands for engine health, forward kinematics,
  and restart; do not expose the shell plugin to the webview.
- Keep the lifecycle-owning `robot-engine-client` behind Tauri managed state.
- Bundle `robot-engine-cli` as a Tauri external binary named
  `robot-engine-cli-$TARGET_TRIPLE` (plus `.exe` on Windows).
- Convert Tauri's resolved sidecar command into a standard process command and
  pass it to `robot-engine-client`, which configures piped standard streams and
  owns shutdown.
- Return structured, serializable command errors to React instead of raw Rust or
  process errors.

## Consequences

- React cannot spawn arbitrary programs or bypass lifecycle policy.
- The executable and its native libraries must be staged and inspected for each
  target during packaging.
- Desktop end-to-end tests must run where child processes are supported.
- Frontend validation and production asset builds remain independently testable
  without launching the engine.

## Alternatives considered

- Grant shell permissions to React: rejected because it expands the trusted UI
  surface and duplicates Rust lifecycle responsibilities.
- Hard-code an engine path: rejected because development and packaged paths
  differ by platform and target triple.
- Link C++ into the Tauri process: rejected by ADR-0002's crash-isolation and ABI
  requirements.
