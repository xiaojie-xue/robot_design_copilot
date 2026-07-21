# ADR-0001: Language Boundaries

Status: Accepted  
Date: 2026-07-21

## Context

The product needs desktop integration, AI providers, deterministic robot calculations, and interactive 3D. The selected engineering libraries are C++ libraries; Tauri requires Rust; React and Three.js use TypeScript.

## Decision

- TypeScript owns React UI, editing, result presentation, and Three.js.
- Rust owns Tauri, application use cases, project files, SQLite, keychain access, AI providers, jobs, and engine process management.
- C++20 owns robot models, kinematics, dynamics, collision, optimization, simulation adapters, and deterministic engineering checks.
- Go is excluded from the MVP. Reconsider it only for a separately deployed network service.

Engineering algorithms have one C++ implementation. Rust may validate protocol structure but must not duplicate FK, IK, dynamics, collision, or optimization logic. AI output is never an engineering result.

## Consequences

- Each language has one distinct responsibility.
- The engineering core remains independently testable.
- The repository must maintain three toolchains and a versioned protocol.
- Language-private types cannot cross process boundaries.

## Alternatives considered

- All Rust: rejected because it would replace or wrap the selected C++ robotics stack.
- Python engineering service: rejected to avoid bundling Python and native extensions.
- Go in the desktop app: rejected because Rust already owns those responsibilities.
