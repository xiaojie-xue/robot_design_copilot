# ADR-0002: C++ Sidecar and Standard-stream IPC

Status: Accepted  
Date: 2026-07-21

## Context

Rust must call the C++ engineering core across all supported platforms. The boundary must support cancellation and isolate native crashes from the UI.

## Decision

Build two C++ targets:

- `robot-engine-core`: a Tauri-independent library.
- `robot-engine-cli`: a thin executable linked to the core.

Tauri starts the executable as a child process. Rust and C++ exchange length-prefixed UTF-8 JSON messages over stdin/stdout. The protocol includes a version, request ID, method, parameters, result or structured error, progress events, cancellation, and engine version.

Stdout carries protocol frames only. Stderr carries logs only. Rust owns startup, request matching, timeouts, cancellation, crash detection, restart, and shutdown.

## Consequences

- A C++ crash does not terminate the UI process.
- No local port or HTTP authentication is required.
- All boundary data must be serializable.
- Large meshes and trajectories may require a later binary or file-based transfer path.
- CI must package native dependencies on each platform.

## Alternatives considered

- Rust/C++ FFI: rejected for the MVP because of ABI, exception, compiler, and crash-isolation costs.
- Local HTTP/gRPC: rejected because a single parent/child pair does not require a network service.
- One process per calculation: rejected because repeated model loading prevents responsive interaction.
